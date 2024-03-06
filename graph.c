#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "container.h"
#include "logging.h"
#include "graph.h"

bld_edges   new_edges();
void        free_edges(bld_edges*);
void        append_edge(bld_edges*, uintmax_t);
void        add_edge(bld_node*, uintmax_t);

bld_nodes   new_nodes();
void        free_nodes(bld_nodes*);
void        push_node(bld_nodes*, bld_node);
bld_node    new_node(bld_file*);
void        free_node(bld_node*);

bld_funcs   new_funcs();
void        free_funcs(bld_funcs*);
void        add_func(bld_funcs*, bld_string*);

typedef struct bld_stack {
    bld_array array;
} bld_stack;

struct bld_search_info {
    bld_graph* graph;
    bld_stack stack;
    bld_set visited;
};

void node_push(bld_stack* stack, bld_node* node) {
    bld_array_push(&stack->array, &node, sizeof(bld_node*));
}

bld_node* node_pop(bld_stack* stack) {
    bld_node* node;
    bld_array_pop(&stack->array, &node, sizeof(bld_node*));
    return node;
}

bld_search_info* graph_dfs_from(bld_graph* graph, bld_file* main) {
    bld_node *node;
    bld_search_info* info = malloc(sizeof(bld_search_info));
    bld_iter iter;
    if (info == NULL) {
        log_fatal("Could not allocate info to start search.");
    }

    info->graph = graph;
    info->stack = (bld_stack) {.array = bld_array_new()};
    info->visited = bld_set_new();

    iter = bld_iter_set(&graph->nodes.set, sizeof(bld_node));
    while (bld_set_next(&iter, (void**) &node)) {
        if (file_eq(node->file, main)) {
            node_push(&info->stack, node);
            break;
        }
    }

    return info;
}

void free_info(bld_search_info* info) {
    bld_array_free(&info->stack.array);
    bld_set_free(&info->visited);
    free(info);
}

int next_file(bld_search_info* info, bld_file** file) {
    int node_visited = 1;
    uintmax_t* index;
    bld_node *node, *to_node;
    bld_iter iter;

    while (node_visited) {
        if (info->stack.array.size <= 0) {
            free_info(info);
            return 0;
        }

        node = node_pop(&info->stack);

        node_visited = 0;
        if (bld_set_has(&info->visited, node->file->identifier.hash)) {
            node_visited = 1;
            goto next_node;
        }

        bld_set_add(&info->visited, node->file->identifier.hash, &node, sizeof(bld_node*));

        iter = bld_iter_array(&node->edges.array, sizeof(uintmax_t));
        while (bld_array_next(&iter, (void**) &index)) {
            to_node = bld_set_get(&info->graph->nodes.set, *index, sizeof(bld_node));
            node_push(&info->stack, to_node);
        }

        next_node:;
    }

    *file = node->file;

    return 1;
}

bld_graph new_graph(bld_files* files) {
    return (bld_graph) {
        .files = files,
        .nodes = new_nodes(),
    };
}

void free_graph(bld_graph* graph) {
    free_nodes(&graph->nodes);
}

bld_node parse_symbols(bld_file* file, bld_path* symbol_path) {
    FILE* f;
    int c, func_type;
    bld_string func;
    bld_node node;

    node = (bld_node) {
        .file = file,
        .defined_funcs = new_funcs(),
        .used_funcs = new_funcs(),
        .edges = new_edges(),
    };

    f = fopen(path_to_string(symbol_path), "r");
    if (f == NULL) {log_fatal("parse_symbols: symbol file could not be opened");}

    while (1) {
        c = fgetc(f);
        if (c == EOF) {
            log_warn("unexpected EOF when parsing symbols of \"%s\"", path_to_string(&node.file->path));
            break;
        }

        while (c != EOF && c != ' ') {c = fgetc(f);}
        if (c == EOF) {
            log_warn("unexpected EOF when parsing symbols of \"%s\"", path_to_string(&node.file->path));
            break;
        }
        while (c != EOF && c == ' ') {c = fgetc(f);}
        if (c == EOF) {
            log_warn("unexpected EOF when parsing symbols of \"%s\"", path_to_string(&node.file->path));
            break;
        }

        func_type = c;
        if (func_type != 'T' && func_type != 'U') {goto next_line;}

        c = fgetc(f);
        if (c != ' ') {
            log_warn("Unexpected character \'%c\' when parsing symbols, expected \' \'", c);
            goto next_line;
        }

        func = new_string();
        c = fgetc(f);
        while (c != '\n' && c != ' ' && c != EOF) {
            append_char(&func, c);
            c = fgetc(f);
        }

        if (func_type == 'T') {
            add_func(&node.defined_funcs, &func);
        } else if (func_type == 'U') {
            add_func(&node.used_funcs, &func);
        } else {
            log_fatal("parse_symbols: unreachable error");
        }

        next_line:
        while (c != '\n' && c != EOF) {c = fgetc(f);}
        c = fgetc(f);
        if (c == EOF) {break;}
    }

    fclose(f);
    return node;
}

void populate_node(bld_graph* graph, bld_path* cache_path, bld_path* symbol_path, bld_file* file) {
    int result;
    char name[256];
    bld_string cmd = new_string();
    bld_path path;
    bld_node node;

    if (file->type == BLD_HEADER) {
        free_string(&cmd);
        return;
    }

    fclose(fopen(path_to_string(symbol_path), "w"));

    append_string(&cmd, "nm ");

    path = copy_path(cache_path);
    serialize_identifier(name, file);
    append_dir(&path, name);
    append_string(&cmd, path_to_string(&path));
    append_string(&cmd, ".o");
    free_path(&path);

    append_string(&cmd, " >> ");
    append_string(&cmd, path_to_string(symbol_path));
    
    result = system(make_string(&cmd));
    if (result) {
        log_fatal("Unable to extract symbols from \"%s\"", path_to_string(&file->path));
    }

    node = parse_symbols(file, symbol_path);
    log_debug("Populating: \"%s\", %lu function(s), %lu reference(s)", path_to_string(&file->path), node.defined_funcs.set.size, node.used_funcs.set.size);
    push_node(&graph->nodes, node);

    free_string(&cmd);
}

void connect_node(bld_graph* graph, bld_node* node) {
    bld_iter iter = bld_iter_set(&graph->nodes.set, sizeof(bld_node));
    bld_node* to_node;

    while (bld_set_next(&iter, (void**) &to_node)) {
        if (bld_set_empty_intersection(&node->used_funcs.set, &to_node->defined_funcs.set)) {
            continue;
        }
        add_edge(node, to_node->file->identifier.id);
    }
}

void generate_graph(bld_graph* graph, bld_path* cache_path) {
    bld_path symbol_path;
    bld_iter iter;
    bld_node *node;
    bld_file* file;
    /* TODO: utilize cache if present */

    symbol_path = copy_path(cache_path);
    append_dir(&symbol_path, "symbols");

    iter = bld_iter_set(&graph->files->set, sizeof(bld_file));
    while (bld_set_next(&iter, (void**) &file)) {
        populate_node(graph, cache_path, &symbol_path, file);
    }

    remove(path_to_string(&symbol_path));

    iter = bld_iter_set(&graph->nodes.set, sizeof(bld_node));
    while (bld_set_next(&iter, (void**) &node)) {
        connect_node(graph, node);
    }

    free_path(&symbol_path);
    log_info("Generated dependency graph with %lu nodes", graph->nodes.set.size);
}

bld_edges new_edges() {
    return (bld_edges) {.array = bld_array_new()};
}

void free_edges(bld_edges* edges) {
    bld_array_free(&edges->array);
}

void add_edge(bld_node* from, uintmax_t to_index) {
    append_edge(&from->edges, to_index);
}

void append_edge(bld_edges* edges, uintmax_t to_index) {
    bld_array_push(&edges->array, &to_index, sizeof(uintmax_t));
}

bld_nodes new_nodes() {
    return (bld_nodes) {.set = bld_set_new()};
}

void free_nodes(bld_nodes* nodes) {
    bld_iter iter = bld_iter_set(&nodes->set, sizeof(bld_node));
    bld_node* node;

    while (bld_set_next(&iter, (void**) &node)) {
        free_node(node);
    }
    bld_set_free(&nodes->set);
}

void push_node(bld_nodes* nodes, bld_node node) {
    bld_set_add(&nodes->set, node.file->identifier.id, &node, sizeof(bld_node));
}

bld_node new_node(bld_file* file) {
    return (bld_node) {
        .file = file,
        .defined_funcs = new_funcs(),
        .used_funcs = new_funcs(),
        .edges = new_edges(),
    };
}

void free_node(bld_node* node) {
    free_funcs(&node->defined_funcs);
    free_funcs(&node->used_funcs);
    free_edges(&node->edges);
}

bld_funcs new_funcs() {
    return (bld_funcs) {.set = bld_set_new()};
}

void free_funcs(bld_funcs* funcs) {
    char** func_ptr = funcs->set.values;
    for (size_t i = 0; i < funcs->set.capacity + funcs->set.max_offset; i++) {
        if (funcs->set.offset[i] >= funcs->set.max_offset) {continue;}
        free(func_ptr[i]);
    }
    bld_set_free(&funcs->set);
}

void add_func(bld_funcs* funcs, bld_string* func) {
    char* str = make_string(func);
    bld_set_add(&funcs->set, hash_string(str, 0), &str, sizeof(char*));
}
