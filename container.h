#ifndef CONTAINER_H
#define CONTAINER_H
#include <stdlib.h>
#include <inttypes.h>

typedef uintmax_t bld_hash;
typedef size_t bld_offset;

typedef struct bld_array {
    size_t capacity;
    size_t size;
    size_t value_size;
    void* values;
} bld_array;

typedef struct bld_set {
    size_t capacity;
    size_t size;
    size_t value_size;
    size_t max_offset;
    bld_offset* offset;
    bld_hash* hash;
    void* values;
} bld_set;

typedef struct bld_graph {
    bld_set edges;
} bld_graph;

enum bld_container_type {
    BLD_ARRAY,
    BLD_SET,
    BLD_GRAPH,
};

enum bld_graph_search_type {
    BLD_GRAPH_DONE,
    BLD_DFS,
};

typedef struct bld_iter_array {
    const bld_array* array;
    size_t index;
} bld_iter_array;

typedef struct bld_iter_set {
    const bld_set* set;
    size_t index;
} bld_iter_set;

typedef struct bld_iter_graph {
    enum bld_graph_search_type type;
    const bld_graph* graph;
    bld_array stack;
    bld_set visited;
} bld_iter_graph;

union bld_iter_container {
    bld_iter_array array_iter;
    bld_iter_set set_iter;
    bld_iter_graph graph_iter;
};

typedef struct bld_iter {
    enum bld_container_type type;
    union bld_iter_container as;
} bld_iter;

bld_array   array_new(size_t);
void        array_free(bld_array*);
bld_array   array_copy(const bld_array*);
void        array_push(bld_array*, void*);
void*       array_pop(bld_array*);
int         array_next(bld_iter_array*, void**);

bld_set     set_new(size_t);
void        set_free(bld_set*);
bld_set     set_copy(const bld_set*);
void        set_clear(bld_set*);
int         set_add(bld_set*, bld_hash, void*);
void*       set_get(const bld_set*, bld_hash);
int         set_has(const bld_set*, bld_hash);
int         set_empty_intersection(const bld_set*, const bld_set*);
int         set_next(bld_iter_set*, void**);

bld_graph   graph_new(void);
void        graph_free(bld_graph*);
void        graph_add_node(bld_graph*, uintmax_t);
void        graph_add_edge(bld_graph*, uintmax_t, uintmax_t);
int         graph_has_node(const bld_graph*, uintmax_t);
int         graph_next(bld_iter_graph*, uintmax_t*);

bld_iter    iter_array(const bld_array*);
bld_iter    iter_set(const bld_set*);
bld_iter    iter_graph(const bld_graph*, uintmax_t);
int         iter_next(bld_iter*, void**);

#endif
