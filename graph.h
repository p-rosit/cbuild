#ifndef GRAPH_H
#define GRAPH_H
#include "set.h"

typedef struct bit_graph {
    bit_set edges;
} bit_graph;

bit_graph   graph_new(void);
void        graph_free(bit_graph*);
void        graph_add_node(bit_graph*, uintmax_t);
void        graph_add_edge(bit_graph*, uintmax_t, uintmax_t);
int         graph_has_node(const bit_graph*, uintmax_t);

#endif
