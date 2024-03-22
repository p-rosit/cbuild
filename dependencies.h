#ifndef GRAPH_H
#define GRAPH_H

#include "path.h"
#include "file.h"

typedef struct bld_search_info bld_search_info;
typedef struct bld_node bld_node;

struct bld_node {
    uintmax_t file_id;
    bld_array functions_from;
    bld_array included_in;
};

typedef struct bld_graph {
    bld_set* files;
    bld_set nodes;
} bld_graph;

bld_graph           graph_t_new(bld_set*);
void                graph_t_free(bld_graph*);

void                graph_generate(bld_graph*, bld_path*);
bld_search_info*    graph_functions_from(bld_graph*, bld_file*);
bld_search_info*    graph_includes_from(bld_graph*, bld_file*);
int                 graph_next_file(bld_search_info*, bld_file**);

#endif