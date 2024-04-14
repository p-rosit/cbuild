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

bld_tree_node tree_push(bld_tree* tree, bld_tree_node* p, void* value) {
    bld_tree_node node, *parent, *sibling;

    node.parent = p->size - p->self;
    node.next = 1;
    node.self = p->self + p->size;
    node.last = 0;
    node.size = 0;

    array_insert(&tree->nodes, node.self, &node);
    array_insert(&tree->data, node.self, value);

    parent = array_get(&tree->nodes, p->self);
    sibling = array_get(&tree->nodes, p->self + p->size - 1);

    parent->size += 1;
    sibling->next = 0;

    return node;
}
