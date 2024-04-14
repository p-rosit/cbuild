#ifndef TREE_H
#define TREE_H
#include <stdlib.h>
#include "array.h"

typedef struct bld_tree_node {
    size_t parent;
    size_t self;
    size_t next;
    size_t size;
} bld_tree_node;

typedef struct bld_tree {
    bld_array nodes;
    bld_array data;
} bld_tree;


bld_tree        tree_new(size_t);
void            tree_free(bld_tree*);
bld_tree_node   tree_push(bld_tree*, bld_tree_node*, void*);

#endif
