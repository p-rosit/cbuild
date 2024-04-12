#ifndef GRAPH_H
#define GRAPH_H
#include "set.h"

typedef struct bld_graph {
    bld_set edges;
} bld_graph;

bld_graph   graph_new(void);
void        graph_free(bld_graph*);
void        graph_add_node(bld_graph*, uintmax_t);
void        graph_add_edge(bld_graph*, uintmax_t, uintmax_t);
int         graph_has_node(const bld_graph*, uintmax_t);

#endif
