#include "os.h"
#include "file_tree.h"

bld_file_tree file_tree_new(void) {
    bld_file_tree tree;

    tree.root = BLD_INVALID_IDENITIFIER;
    tree.parents = set_new(sizeof(uintmax_t));
    tree.tree = graph_new();

    return tree;
}

void file_tree_free(bld_file_tree* tree) {
    set_free(&tree->parents);
    graph_free(&tree->tree);
}

void file_tree_set_root(bld_file_tree* tree, uintmax_t root) {
    tree->root = root;
    graph_add_node(&tree->tree, root);
}

void file_tree_add(bld_file_tree* tree, uintmax_t parent, uintmax_t file) {
    set_add(&tree->parents, file, &parent);
    graph_add_node(&tree->tree, file);
    graph_add_edge(&tree->tree, parent, file);
}

uintmax_t file_tree_get_parent(bld_file_tree* tree, uintmax_t file) {
    uintmax_t* parent = set_get(&tree->parents, file);

    if (parent != NULL) {
        return *parent;
    } else {
        return BLD_INVALID_IDENITIFIER;
    }
}

bld_iter file_tree_files(bld_file_tree* tree, uintmax_t root) {
    return iter_graph(&tree->tree, root);
}

bld_iter file_tree_children(bld_file_tree* tree, uintmax_t parent) {
    return iter_graph_children(&tree->tree, parent);
}
