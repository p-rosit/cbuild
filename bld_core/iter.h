#ifndef ITER_H
#define ITER_H
#include "os.h"
#include "dstr.h"
#include "array.h"
#include "set.h"
#include "graph.h"

enum bld_container_type {
    BLD_ARRAY,
    BLD_SET,
    BLD_GRAPH,
    BLD_DIRECTORY
};

enum bld_graph_search_type {
    BLD_GRAPH_DONE,
    BLD_DFS
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

typedef struct bld_iter_directory {
    bld_os_dir* dir;
} bld_iter_directory;

union bld_iter_container {
    bld_iter_array array_iter;
    bld_iter_set set_iter;
    bld_iter_graph graph_iter;
    bld_iter_directory directory_iter;
};

typedef struct bld_iter {
    enum bld_container_type type;
    union bld_iter_container as;
} bld_iter;

bld_iter    iter_array(const bld_array*);
bld_iter    iter_set(const bld_set*);
bld_iter    iter_graph(const bld_graph*, uintmax_t);
bld_iter    iter_graph_children(const bld_graph*, uintmax_t);
bld_iter    iter_directory(bld_os_dir*);
int         iter_next(bld_iter*, void**);
int         array_next(bld_iter_array*, void**);
int         set_next(bld_iter_set*, void**);
int         graph_next(bld_iter_graph*, uintmax_t*);
int         directory_next(bld_iter_directory*, bld_string*);

#endif
