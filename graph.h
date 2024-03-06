#ifndef GRAPH_H
#define GRAPH_H

#include "path.h"
#include "file.h"

typedef struct bld_search_info bld_search_info;
typedef struct bld_node bld_node;

typedef struct bld_funcs {
    bld_set set;
} bld_funcs;

typedef struct bld_edges {
    bld_array array;
} bld_edges;

struct bld_node {
    bld_file* file;
    bld_funcs defined_funcs;
    bld_funcs used_funcs;
    bld_edges edges;
};

typedef struct bld_nodes {
    bld_set set;
} bld_nodes;

typedef struct bld_graph {
    bld_files* files;
    bld_nodes nodes;
} bld_graph;

bld_graph           new_graph(bld_files*);
void                free_graph(bld_graph*);

void                generate_graph(bld_graph*, bld_path*);
bld_search_info*    graph_dfs_from(bld_graph*, bld_file*);
int                 next_file(bld_search_info*, bld_file**);

#endif
