#ifndef GRAPH_H
#define GRAPH_H

#include "path.h"
#include "file.h"

typedef struct bld_search_info bld_search_info;
typedef struct bld_node bld_node;

typedef struct bld_funcs {
    /* TODO: Hash set for faster lookup*/
    size_t capacity;
    size_t size;
    char** funcs;
} bld_funcs;

typedef struct bld_edges {
    /* TODO: hash set for faster lookup */
    size_t capacity;
    size_t size;
    bld_node** nodes;
} bld_edges;

struct bld_node {
    bld_file* file;
    bld_funcs defined_funcs;
    bld_funcs used_funcs;
    bld_edges edges;
};

typedef struct bld_graph {
    bld_files* files;
    size_t size;
    bld_node* nodes; /* TODO: growable array? */
} bld_graph;

bld_graph   new_graph(bld_files*);
void        free_graph(bld_graph*);

void generate_graph(bld_graph*, bld_path*);
bld_search_info* graph_dfs_from(bld_graph*, bld_file*);
int next_file(bld_search_info*, bld_file**);

#endif
