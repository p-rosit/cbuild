#ifndef ITER_H
#define ITER_H
#include "array.h"
#include "set.h"
#include "graph.h"

enum bit_container_type {
    BIT_ARRAY,
    BIT_SET,
    BIT_GRAPH
};

enum bit_graph_search_type {
    BIT_GRAPH_DONE,
    BIT_DFS
};

typedef struct bit_iter_array {
    const bit_array* array;
    size_t index;
} bit_iter_array;

typedef struct bit_iter_set {
    const bit_set* set;
    size_t index;
} bit_iter_set;

typedef struct bit_iter_graph {
    enum bit_graph_search_type type;
    const bit_graph* graph;
    bit_array stack;
    bit_set visited;
} bit_iter_graph;

union bit_iter_container {
    bit_iter_array array_iter;
    bit_iter_set set_iter;
    bit_iter_graph graph_iter;
};

typedef struct bit_iter {
    enum bit_container_type type;
    union bit_iter_container as;
} bit_iter;

bit_iter    iter_array(const bit_array*);
bit_iter    iter_set(const bit_set*);
bit_iter    iter_graph(const bit_graph*, uintmax_t);
bit_iter    iter_graph_children(const bit_graph*, uintmax_t);
int         iter_next(bit_iter*, void**);
int         array_next(bit_iter_array*, void**);
int         set_next(bit_iter_set*, void**);
int         graph_next(bit_iter_graph*, uintmax_t*);

#endif
