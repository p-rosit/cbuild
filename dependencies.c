#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "container.h"
#include "logging.h"
#include "dependencies.h"

void parse_included_files(bld_file*);

void parse_symbols(bld_file*, bld_path*);
void generate_symbol_file(bld_file*, bld_path*, bld_path*);
void add_symbol(bld_set*, bld_string*);

bld_dependency_graph dependency_graph_new(bld_set* files) {
    return (bld_dependency_graph) {
        .files = files,
        .include_graph = graph_new(),
        .symbol_graph = graph_new(),
    };
}

void dependency_graph_free(bld_dependency_graph* graph) {
    graph_free(&graph->include_graph);
    graph_free(&graph->symbol_graph);
}

bld_iter dependency_graph_symbols_from(const bld_dependency_graph* graph, bld_file* root) {
    return iter_graph(&graph->symbol_graph, root->identifier.id);
}

bld_iter dependency_graph_includes_from(const bld_dependency_graph* graph, bld_file* root) {
    return  iter_graph(&graph->include_graph, root->identifier.id);
}

int dependency_graph_next_file(bld_iter* iter, const bld_dependency_graph* graph, bld_file** file) {
    uintmax_t file_id;
    int has_next;
    
    has_next = graph_next(&iter->as.graph_iter, &file_id);
    if (!has_next) {return has_next;}

    *file = set_get(graph->files, file_id);
    if (*file == NULL) {log_fatal("File id in graph did not exist in file set.");}

    return has_next;
}

void dependency_graph_extract_includes(bld_dependency_graph* graph) {
    bld_iter iter;
    bld_file *file;
    log_debug("Extracting includes, files in cache: %lu/%lu", graph->include_graph.edges.size, graph->files->size);

    iter = iter_set(graph->files);
    while (iter_next(&iter, (void**) &file)) {
        if (graph_has_node(&graph->include_graph, file->identifier.id)) {
            continue;
        }
        log_debug("Extracting includes of \"%s\"", string_unpack(&file->name));
        graph_add_node(&graph->include_graph, file->identifier.id);
        parse_included_files(file);
    }

    iter = iter_set(graph->files);
    while (iter_next(&iter, (void**) &file)) {
        bld_iter iter;
        bld_file *to_file;

        iter = iter_set(graph->files);
        while (iter_next(&iter, (void**) &to_file)) {
            if (!set_has(&to_file->includes, file->identifier.id)) {
                continue;
            }
            graph_add_edge(&graph->include_graph, file->identifier.id, to_file->identifier.id);
        }
    }

    log_info("Generated include graph with %lu nodes", graph->include_graph.edges.size);
}

void dependency_graph_extract_symbols(bld_dependency_graph* graph, bld_path* cache_path) {
    bld_path symbol_path;
    bld_iter iter;
    bld_file* file;
    log_debug("Extracting symbols, files in cache: %lu/%lu", graph->symbol_graph.edges.size, graph->files->size);

    symbol_path = path_copy(cache_path);
    path_append_string(&symbol_path, "symbols");

    iter = iter_set(graph->files);
    while (iter_next(&iter, (void**) &file)) {
        if (file->type == BLD_HEADER) {continue;}

        if (graph_has_node(&graph->symbol_graph, file->identifier.id)) {
            continue;
        }

        log_debug("Extracting symbols of \"%s\"", string_unpack(&file->name));
        graph_add_node(&graph->symbol_graph, file->identifier.id);

        generate_symbol_file(file, cache_path, &symbol_path);
        parse_symbols(file, &symbol_path);
    }

    remove(path_to_string(&symbol_path));
    path_free(&symbol_path);

    iter = iter_set(graph->files);
    while (iter_next(&iter, (void**) &file)) {
        bld_iter iter;
        bld_file* to_file;

        if (file->type == BLD_HEADER) {continue;}
        iter = iter_set(graph->files);
        while (iter_next(&iter, (void**) &to_file)) {
            if (set_empty_intersection(&file->undefined_symbols, &to_file->defined_symbols)) {
                continue;
            }
            graph_add_edge(&graph->symbol_graph, file->identifier.id, to_file->identifier.id);
        }
    }

    log_info("Generated symbol graph with %lu nodes", graph->symbol_graph.edges.size);
}

void generate_symbol_file(bld_file* file, bld_path* cache_path, bld_path* symbol_path) {
    int result;
    bld_string cmd;
    bld_path path;
    char name[FILENAME_MAX];

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

    string_free(&cmd);
}

void parse_symbols(bld_file* file, bld_path* symbol_path) {
    FILE* f;
    int c, symbol_type;
    bld_string func;

    f = fopen(path_to_string(symbol_path), "r");
    if (f == NULL) {log_fatal("parse_symbols: symbol file could not be opened");}

    // log_warn("Parsing: \"%s\"", string_unpack(&file->name));
    while (1) {
        c = fgetc(f);
        if (c == EOF) {
            log_warn("Unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
            break;
        }

        while (c != EOF && c != ' ') {c = fgetc(f);}
        if (c == EOF) {
            log_warn("Unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
            break;
        }
        while (c != EOF && c == ' ') {c = fgetc(f);}
        if (c == EOF) {
            log_warn("Unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
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

void add_symbol(bld_set* set, bld_string* str) {
    char* temp = string_unpack(str);
    set_add(set, string_hash(temp, 0), &temp);
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
