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

bld_tree_node tree_push(bld_tree* tree, bld_tree_node* parent, void* value) {
    bld_tree_node node;

    node.parent = parent->self;
    node.self = parent->self + parent->size;
    node.next = node.self;
    node.size = 0;

    array_insert(&tree->nodes, node.self, &node);
    array_insert(&tree->data, node.self, value);

    return node;
}
