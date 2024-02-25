#include <ctype.h>
#include <string.h>
#include "build.h"
#include "graph.h"

bld_edges   new_edges();
void        free_edges(bld_edges*);
int         append_edge(bld_edges*, uintmax_t);
int         add_edge(bld_node*, uintmax_t);

bld_nodes   new_nodes();
void        free_nodes(bld_nodes*);
void        push_node(bld_nodes*, bld_node);
bld_node    new_node(bld_file*);
void        free_node(bld_node*);

bld_funcs   new_funcs();
void        free_funcs(bld_funcs*);
void        add_func(bld_funcs*, bld_string*);

typedef struct bld_stack {
    size_t capacity;
    size_t size;
    bld_node** nodes;
} bld_stack;

struct bld_search_info {
    bld_graph* graph;
    bld_stack stack;
    bld_stack visited; /* TODO: hash set for faster lookup */
};

void node_push(bld_stack* stack, bld_node* node) {
    size_t capacity = stack->capacity;
    bld_node** nodes;

    if (stack->size >= stack->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        nodes = malloc(capacity * sizeof(bld_node*));
        if (nodes == NULL) {log_fatal("Could not push node");}
        memcpy(nodes, stack->nodes, stack->size * sizeof(bld_node*));
        free(stack->nodes);

        stack->nodes = nodes;
        stack->capacity = capacity;
    }

    stack->nodes[stack->size++] = node;
}

bld_node* node_pop(bld_stack* stack) {
    if (stack->size <= 0) {log_fatal("Trying to pop from empty stack");}
    return stack->nodes[--stack->size];
}

bld_search_info* graph_dfs_from(bld_graph* graph, bld_file* main) {
    bld_node* node;
    bld_search_info* info = malloc(sizeof(bld_search_info));
    if (info == NULL) {
        log_fatal("Could not allocate info to start search.");
    }

    info->graph = graph;
    info->stack = (bld_stack) {
        .capacity = 0,
        .size = 0,
        .nodes = NULL,
    };
    info->visited = (bld_stack) {
        .capacity = 0,
        .size = 0,
        .nodes = NULL,
    };

    for (size_t i = 0; i < graph->nodes.size; i++) {
        node = &graph->nodes.nodes[i];
        if (file_eq(node->file, main)) {
            node_push(&info->stack, node);
            break;
        }
    }

    return info;
}

void free_info(bld_search_info* info) {
    free(info->stack.nodes);
    free(info->visited.nodes);
    free(info);
}

int next_file(bld_search_info* info, bld_file** file) {
    int visited = 1;
    uintmax_t index;
    bld_node* node;

    while (visited) {
        if (info->stack.size <= 0) {
            free_info(info);
            return 0;
        }

        node = node_pop(&info->stack);

        visited = 0;
        for (size_t i = 0; i < info->visited.size; i++) {
            if (node == info->visited.nodes[i]) {
                visited = 1;
                goto next_node;
            }
        }

        node_push(&info->visited, node);

        for (size_t i = 0; i < node->edges.size; i++) {
            index = node->edges.indices[i];
            node_push(&info->stack, &info->graph->nodes.nodes[index]);
        }

        next_node:;
    }

    *file = node->file;

    return 1;
}

bld_graph new_graph(bld_files* files) {
    size_t capacity;
    if (files == NULL) {
        capacity = 0;
    } else {
        capacity = files->size;
    }

    return (bld_graph) {
        .files = files,
        .nodes = new_nodes(capacity),
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
    bld_node* node;

    if (file->type == BLD_HEADER) {
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

    push_node(&graph->nodes, parse_symbols(file, symbol_path));
    node = &graph->nodes.nodes[graph->nodes.size - 1];
    log_debug("Populating: \"%s\", %lu function(s), %lu reference(s)", path_to_string(&file->path), node->defined_funcs.size, node->used_funcs.size);

    free_string(&cmd);
}

void connect_node(bld_graph* graph, bld_node* node) {
    char *undef, *def;
    bld_node* to_node;

    for (size_t i = 0; i < graph->nodes.size; i++) {
        to_node = &graph->nodes.nodes[i];

        for (size_t j = 0; j < node->used_funcs.size; j++) {
            undef = node->used_funcs.funcs[j];
            for (size_t k = 0; k < to_node->defined_funcs.size; k++) {
                def = to_node->defined_funcs.funcs[k];
                if (strcmp(undef, def) == 0) {
                    if (add_edge(node, i)) {
                        log_fatal("Could not add edge from \"%s\" to \"%s\"", make_string(&node->file->name), make_string(&to_node->file->name));
                    }
                    goto next_node;
                }
            }
        }

        next_node:;
    }
}

void generate_graph(bld_graph* graph, bld_path* cache_path) {
    bld_path symbol_path;

    symbol_path = copy_path(cache_path);
    append_dir(&symbol_path, "symbols");

    for (size_t i = 0; i < graph->files->size; i++) {
        populate_node(graph, cache_path, &symbol_path, &graph->files->files[i]);
    }

    remove(path_to_string(&symbol_path));

    for (size_t i = 0; i < graph->nodes.size; i++) {
        connect_node(graph, &graph->nodes.nodes[i]);
    }

    free_path(&symbol_path);
    log_info("Generated dependency graph with %lu nodes", graph->nodes.size);
}

bld_edges new_edges() {
    return (bld_edges) {
        .capacity = 0,
        .size = 0,
        .indices = NULL,
    };
}

void free_edges(bld_edges* edges) {
    free(edges->indices);
}

int add_edge(bld_node* from, uintmax_t to_index) {
    return append_edge(&from->edges, to_index);
}

int append_edge(bld_edges* edges, uintmax_t to_index) {
    size_t capacity = edges->capacity;
    uintmax_t* indices;

    if (edges->size >= edges->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        indices = malloc(capacity * sizeof(bld_node*));
        if (indices == NULL) {return -1;}

        memcpy(indices, edges->indices, edges->size * sizeof(uintmax_t));
        free(edges->indices);

        edges->indices = indices;
        edges->capacity = capacity;
    }

    edges->indices[edges->size++] = to_index;
    return 0;
}

bld_nodes new_nodes() {
    return (bld_nodes) {
        .capacity = 0,
        .size = 0,
        .nodes = NULL,
    };
}

void free_nodes(bld_nodes* nodes) {
    for (size_t i = 0; i < nodes->size; i++) {
        free_node(&nodes->nodes[i]);
    }
    free(nodes->nodes);
}

void push_node(bld_nodes* nodes, bld_node node) {
    size_t capacity = nodes->capacity;
    bld_node* temp;

    if (nodes->size >= nodes->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        temp = malloc(capacity * sizeof(bld_node));
        if (temp == NULL) {log_fatal("Could not push node");}

        memcpy(temp, nodes->nodes, nodes->size * sizeof(bld_node));
        free(nodes->nodes);

        nodes->capacity = capacity;
        nodes->nodes = temp;
    }

    nodes->nodes[nodes->size++] = node;
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
    return (bld_funcs) {
        .capacity = 0,
        .size = 0,
        .funcs = NULL,
    };
}

void free_funcs(bld_funcs* funcs) {
    for (size_t i = 0; i < funcs->size; i++) {
        free(funcs->funcs[i]);
    }
    free(funcs->funcs);
}

void add_func(bld_funcs* funcs, bld_string* func) {
    size_t capacity = funcs->capacity;
    char** names;

    if (funcs->size >= funcs->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        names = malloc(capacity * sizeof(char*));
        if (names == NULL) {log_fatal("Could not add function \"%s\"", make_string(func));}

        memcpy(names, funcs->funcs, funcs->size * sizeof(char*));
        free(funcs->funcs);

        funcs->funcs = names;
        funcs->capacity = capacity;
    }

    funcs->funcs[funcs->size++] = make_string(func);
}
