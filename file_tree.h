#ifndef FILE_TREE_H
#define FILE_TREE_H
#include "graph.h"
#include "file.h"
#include "iter.h"

typedef struct bld_file_tree {
    uintmax_t root;
    bld_set parents;
    bld_graph tree;
} bld_file_tree;

bld_file_tree   file_tree_new(void);
void            file_tree_free(bld_file_tree*);
void            file_tree_set_root(bld_file_tree*, uintmax_t);
void            file_tree_add(bld_file_tree*, uintmax_t, uintmax_t);
uintmax_t       file_tree_get_parent(bld_file_tree*, uintmax_t);

bld_iter        file_tree_files(bld_file_tree*, uintmax_t);
bld_iter        file_tree_children(bld_file_tree*, uintmax_t);

#endif
