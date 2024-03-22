#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "container.h"
#include "logging.h"
#include "dependencies.h"

void                connect_node(bld_graph*, bld_node*);
void                populate_node(bld_graph*, bld_path*, bld_path*, bld_file*);
void                parse_symbols(bld_file*, bld_path*);
void                parse_included_files(bld_file*);
bld_search_info*    graph_dfs_from(bld_graph*, bld_file*);
void                add_function_edge(bld_node*, uintmax_t);
void                add_include_edge(bld_node*, uintmax_t);
bld_node            new_node(bld_file*);
void                free_node(bld_node*);
void                add_symbol(bld_set*, bld_string*);

typedef enum bld_search_type {
    BLD_FUNCS,
    BLD_INCLUDES,
    BLD_NO_SEARCH,
} bld_search_type;

struct bld_search_info {
    bld_search_type type;
    bld_graph* graph;
    bld_array stack;
    bld_set visited;
};

void free_info(bld_search_info*);

bld_search_info* graph_dfs_from(bld_graph* graph, bld_file* root) {
    bld_node *node;
    bld_search_info* info = malloc(sizeof(bld_search_info));
    if (info == NULL) {
        log_fatal("Could not allocate info to start search.");
    }

    *info = (bld_search_info) {
        .type = BLD_NO_SEARCH,
        .graph = graph,
        .stack = array_new(sizeof(bld_node*)),
        .visited = set_new(sizeof(uintmax_t)),
    };

    node = set_get(&graph->nodes, root->identifier.id);
    if (node == NULL) {
        log_fatal("Could not find requested file in graph");
    }
    array_push(&info->stack, &node);

    return info;
}

bld_search_info* graph_functions_from(bld_graph* graph, bld_file* root) {
    bld_search_info* info = graph_dfs_from(graph, root);
    info->type = BLD_FUNCS;
    return info;
}

bld_search_info* graph_includes_from(bld_graph* graph, bld_file* root) {
    bld_search_info* info = graph_dfs_from(graph, root);
    info->type = BLD_INCLUDES;
    return info;
}

void free_info(bld_search_info* info) {
    array_free(&info->stack);
    set_free(&info->visited);
    free(info);
}

int graph_next_file(bld_search_info* info, bld_file** file) {
    int node_visited = 1;
    uintmax_t* index;
    bld_array* edge_array;
    bld_node *node, *to_node;
    bld_file* temp;

    if (info->type == BLD_NO_SEARCH) {
        log_fatal("No search type has been set");
    }

    while (node_visited) {
        if (info->stack.size <= 0) {
            free_info(info);
            return 0;
        }

        node = *((bld_node**) array_pop(&info->stack));

        node_visited = 0;
        if (set_has(&info->visited, node->file_id)) {
            node_visited = 1;
            goto next_node;
        }

        set_add(&info->visited, node->file_id, &node);

        switch (info->type) {
            case (BLD_FUNCS): {
                edge_array = &node->functions_from;
            } break;
            case (BLD_INCLUDES): {
                edge_array = &node->included_in;
            } break;
            default: log_fatal("graph_next_file: unreachable error???");
        }

        bld_iter iter = iter_array(edge_array);
        while (iter_next(&iter, (void**) &index)) {
            to_node = set_get(&info->graph->nodes, *index);
            array_push(&info->stack, &to_node);
        }

        next_node:;
    }

    temp = set_get(info->graph->files, node->file_id);
    if (temp == NULL) {log_fatal("File did not exist in graph, internal error");}
    *file = temp;

    return 1;
}

bld_graph graph_t_new(bld_set* files) {
    return (bld_graph) {
        .files = files,
        .nodes = set_new(sizeof(bld_node)),
    };
}

void graph_t_free(bld_graph* graph) {
    bld_node* node;
    bld_iter iter = iter_set(&graph->nodes);

    while (iter_next(&iter, (void**) &node)) {
        free_node(node);
    }
    set_free(&graph->nodes);
}

void parse_symbols(bld_file* file, bld_path* symbol_path) {
    FILE* f;
    int c, symbol_type;
    bld_string func;

    f = fopen(path_to_string(symbol_path), "r");
    if (f == NULL) {log_fatal("parse_symbols: symbol file could not be opened");}

    while (1) {
        c = fgetc(f);
        if (c == EOF) {
            log_warn("unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
            break;
        }

        while (c != EOF && c != ' ') {c = fgetc(f);}
        if (c == EOF) {
            log_warn("unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
            break;
        }
        while (c != EOF && c == ' ') {c = fgetc(f);}
        if (c == EOF) {
            log_warn("unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
            break;
        }

        symbol_type = c;
        if (symbol_type != 'T' && symbol_type != 'B' && symbol_type != 'U') {goto next_line;}

        c = fgetc(f);
        if (c != ' ') {
            log_warn("Unexpected character \'%c\' when parsing symbols, expected \' \'", c);
            goto next_line;
        }

        func = string_new();
        c = fgetc(f);
        while (c != '\n' && c != ' ' && c != EOF) {
            string_append_char(&func, c);
            c = fgetc(f);
        }

        if (symbol_type == 'T' || symbol_type == 'B') {
            add_symbol(&file->defined_symbols, &func);
        } else if (symbol_type == 'U') {
            add_symbol(&file->undefined_symbols, &func);
        } else {
            log_fatal("parse_symbols: unreachable error");
        }

        next_line:
        while (c != '\n' && c != EOF) {c = fgetc(f);}
        c = fgetc(f);
        if (c == EOF) {break;}
    }

    fclose(f);
}

int get_next(FILE*);
int skip_line(FILE*);
int expect_char(FILE*, char);
int expect_string(FILE*, char*);

int get_next(FILE* file) { /* Same as next_character... */
    int c = getc(file);
    while (c != EOF && isspace(c)) {c = getc(file);}
    return c;
}

int skip_line(FILE* file) {
    int c = getc(file);
    while (c != EOF && c != '\n') {c = getc(file);}
    return c == '\n';
}

int expect_char(FILE* file, char c) {
    int file_char = get_next(file);
    return c == file_char;
}

int expect_string(FILE* file, char* str) {
    char str_char;
    int file_char;

    while ((str_char = *str++) != '\0') {
        file_char = getc(file);
        if (file_char != str_char) {return 0;}
    }

    return 1;
}

void parse_included_files(bld_file* file) {
    size_t line_number;
    bld_path parent_path;
    bld_path file_path;
    bld_string str;
    uintmax_t include_id;
    FILE *f, *included_file;
    int c;

    f = fopen(path_to_string(&file->path), "r");
    if (f == NULL) {log_fatal("Could not open file for reading: \"%s\"", string_unpack(&file->name));}

    parent_path = path_copy(&file->path);
    path_remove_last_string(&parent_path);

    line_number = 0;
    while (1) {
        if (!expect_char(f, '#')) {goto next_line;}
        if (!expect_string(f, "include")) {goto next_line;}
        if (!expect_char(f, '\"')) {goto next_line;}

        str = string_new();
        c = getc(f);
        while (c != EOF && c != '\"' && c != '\n') {
            string_append_char(&str, c);
            c = getc(f);
        }
        if (c == EOF) {string_free(&str); break;}
        if (c == '\n') {goto next_line;}

        file_path = path_copy(&parent_path);
        path_append_string(&file_path, string_unpack(&str));

        included_file = fopen(path_to_string(&file_path), "r");
        if (included_file == NULL) {
            log_warn("%s:%lu - Included file \"%s\" is not accessible, ignoring.", path_to_string(&file->path), line_number, string_unpack(&str));
            path_free(&file_path);
            string_free(&str);
            goto next_line;
        }
        fclose(included_file);

        include_id = file_get_id(&file_path);
        set_add(&file->includes, include_id, &include_id);

        path_free(&file_path);
        string_free(&str);

        next_line:
        if (!skip_line(f)) {break;}
        line_number++;
    }

    path_free(&parent_path);
    fclose(f);
}

void populate_node(bld_graph* graph, bld_path* cache_path, bld_path* symbol_path, bld_file* file) {
    int result;
    char name[FILENAME_MAX];
    bld_string cmd;
    bld_path path;
    bld_node node;

    node = new_node(file);

    if (file->type != BLD_HEADER) {
        cmd = string_new();
        fclose(fopen(path_to_string(symbol_path), "w"));

        string_append_string(&cmd, "nm ");

        path = path_copy(cache_path);
        serialize_identifier(name, file);
        path_append_string(&path, name);
        string_append_string(&cmd, path_to_string(&path));
        string_append_string(&cmd, ".o");
        path_free(&path);

        string_append_string(&cmd, " >> ");
        string_append_string(&cmd, path_to_string(symbol_path));

        result = system(string_unpack(&cmd));
        if (result) {
            log_fatal("Unable to extract symbols from \"%s\"", path_to_string(&file->path));
        }
        
        parse_symbols(file, symbol_path);
        string_free(&cmd);
    }

    parse_included_files(file);

    log_debug("Populating: \"%s\", %lu symbol(s), %lu reference(s), %lu include(s)", path_to_string(&file->path), file->defined_symbols.size, file->undefined_symbols.size, file->includes.size);
    set_add(&graph->nodes, node.file_id, &node);

}

void connect_node(bld_graph* graph, bld_node* node) {
    bld_file *file, *to_file;
    bld_node* to_node;

    file = set_get(graph->files, node->file_id);
    if (file == NULL) {log_fatal("Could not get node file, internal error");}

    bld_iter iter = iter_set(&graph->nodes);
    while (iter_next(&iter, (void**) &to_node)) {
        to_file = set_get(graph->files, to_node->file_id);
        if (to_file == NULL) {log_fatal("Could not get to node, internal error");}

        if (!set_empty_intersection(&file->undefined_symbols, &to_file->defined_symbols)) {
            add_function_edge(node, to_file->identifier.id);
        }

        if (set_has(&to_file->includes, file->identifier.id)) {
            add_include_edge(node, to_file->identifier.id);
        }
    }
}

void graph_generate(bld_graph* graph, bld_path* cache_path) {
    bld_path symbol_path;
    bld_node *node;
    bld_file* file;
    /* TODO: utilize cache if present */

    symbol_path = path_copy(cache_path);
    path_append_string(&symbol_path, "symbols");

    bld_iter iter_files = iter_set(graph->files);
    while (iter_next(&iter_files, (void**) &file)) {
        populate_node(graph, cache_path, &symbol_path, file);
    }

    remove(path_to_string(&symbol_path));

    bld_iter iter_nodes = iter_set(&graph->nodes);
    while (iter_next(&iter_nodes, (void**) &node)) {
        connect_node(graph, node);
    }

    path_free(&symbol_path);
    log_info("Generated dependency graph with %lu nodes", graph->nodes.size);
}

void add_function_edge(bld_node* from, uintmax_t file_id) {
    array_push(&from->functions_from, &file_id);
}

void add_include_edge(bld_node* from, uintmax_t file_id) {
    array_push(&from->included_in, &file_id);
}

bld_node new_node(bld_file* file) {
    return (bld_node) {
        .file_id = file->identifier.id,
        .functions_from = array_new(sizeof(uintmax_t)),
        .included_in = array_new(sizeof(uintmax_t)),
    };
}

void free_node(bld_node* node) {
    array_free(&node->functions_from);
    array_free(&node->included_in);
}

void add_symbol(bld_set* set, bld_string* str) {
    char* temp = string_unpack(str);
    set_add(set, string_hash(temp, 0), &temp);
}