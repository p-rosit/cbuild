#include "tree.h"

bld_tree tree_new(size_t value_size) {
    bld_tree tree;

    tree.nodes = array_new(sizeof(bld_tree_node));
    tree.data = array_new(value_size);

    return tree;
}

void tree_free(bld_tree* tree) {
    array_free(&tree->nodes);
    array_free(&tree->data);
}

