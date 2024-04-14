#include "logging.h"
#include "iter.h"

bld_iter iter_array(const bld_array* array) {
    bld_iter iter;

    iter.type = BLD_ARRAY;
    iter.as.array_iter.array = array;
    iter.as.array_iter.index = 0;

    return iter;
}

int array_next(bld_iter_array* iter, void** value_ptr_ptr) {
    size_t value_size = iter->array->value_size;
    const bld_array* array = iter->array;
    char* values = array->values;

    if (iter->index >= array->size) {
        return 0;
    }

    *value_ptr_ptr = values + iter->index++ * value_size;
    return 1;
}

bld_iter iter_set(const bld_set* set) {
    bld_iter iter;

    iter.type = BLD_SET;
    iter.as.set_iter.set = set;
    iter.as.set_iter.index = 0;

    return iter;
}

int set_next(bld_iter_set* iter, void** value_ptr_ptr) {
    int has_next = 0;
    size_t i = iter->index;
    size_t value_size = iter->set->value_size;
    const bld_set* set = iter->set;
    char* values = set->values;
    
    while (i < set->capacity + set->max_offset) {
        if (set->offset[i] < set->max_offset) {
            has_next = 1;
            *value_ptr_ptr = values + i * value_size;
            break;
        }
        i++;
    }

    iter->index = i + 1;
    return has_next;
}

bld_iter iter_tree(const bld_tree* tree, void* root) {
    bld_iter iter;
    char* data;
    size_t index;

    iter.type = BLD_TREE;
    iter.as.tree_iter.type = BLD_TREE_BRANCH;
    iter.as.tree_iter.tree = tree;

    if (root == NULL) {
        iter.as.tree_iter.root = array_get(&tree->nodes, 0);
    } else {
        data = array_get(&tree->data, 0);

        if (((char*) root) < data) {
            log_fatal("iter_tree: invalid pointer, either the pointer to the root was always invalid or the tree was modified during iteration");
        }

        index = ((char*) root) - data;
        if (tree->data.size <= index) {
            log_fatal("iter_tree: invalid pointer, either the pointer to the root was always invalid or the tree was modified during iteration");
        }

        iter.as.tree_iter.root = array_get(&tree->nodes, index);
    }

    iter.as.tree_iter.index = iter.as.tree_iter.root->self;
    return iter;
}

bld_iter iter_tree_children(const bld_tree* tree, void* root) {
    bld_iter iter = iter_tree(tree, root);
    iter.as.tree_iter.type = BLD_TREE_CHILDREN;
    iter.as.tree_iter.index += 1;
    return iter;
}

int tree_next(bld_iter_tree* iter, void** value_ptr_ptr) {
    switch (iter->type) {
        case (BLD_TREE_BRANCH): {
            size_t last_index = iter->root->self + iter->root->size;

            if (iter->index >= last_index) {
                return 0;
            }

            *value_ptr_ptr = array_get(&iter->tree->data, iter->index++);

            return 1;
        }
        case (BLD_TREE_CHILDREN): {
            size_t last_index = iter->root->self + iter->root->last;
            bld_tree_node* node;

            if (iter->index >= last_index) {
                return 0;
            }

            *value_ptr_ptr = array_get(&iter->tree->data, iter->index);
            node = array_get(&iter->tree->nodes, iter->index);

            iter->index += node->next;

            return 1;
        }
    }

    log_fatal("tree_next: unrecognized tree traversal, unreachable error");
    return 0; /* Unreachable */
}

bld_iter iter_graph(const bld_graph* graph, uintmax_t root) {
    bld_iter iter;

    iter.type = BLD_GRAPH;
    iter.as.graph_iter.type = BLD_DFS;
    iter.as.graph_iter.graph = graph;
    iter.as.graph_iter.stack = array_new(sizeof(uintmax_t));
    iter.as.graph_iter.visited = set_new(0);

    if (!set_has(&graph->edges, root)) {log_fatal("Root does not exist");}
    array_push(&iter.as.graph_iter.stack, &root);

    return iter;
}

int graph_next(bld_iter_graph* iter, uintmax_t* node_id) {
    int node_visited = 1;
    uintmax_t *ptr_id, id;
    bld_array* edge_array;

    if (iter->type == BLD_GRAPH_DONE) {
        return 0;
    }

    while (node_visited) {
        bld_iter edges;

        if (iter->stack.size <= 0) {
            iter->type = BLD_GRAPH_DONE;
            array_free(&iter->stack);
            set_free(&iter->visited);
            return 0;
        }

        ptr_id = array_pop(&iter->stack);
        id = *ptr_id;

        node_visited = set_has(&iter->visited, id);
        if (node_visited) {goto next_node;}

        *node_id = id;
        set_add(&iter->visited, id, NULL);

        edge_array = set_get(&iter->graph->edges, id);
        edges = iter_array(edge_array);
        while (iter_next(&edges, (void**) &node_id)) {
            array_push(&iter->stack, node_id);
        }

        next_node:;
    }

    return 1;
}

int iter_next(bld_iter* iter, void** value_ptr_ptr) {
    switch (iter->type) {
        case (BLD_ARRAY): {
            return array_next(&iter->as.array_iter, value_ptr_ptr);
        } break;
        case (BLD_SET): {
            return set_next(&iter->as.set_iter, value_ptr_ptr);
        } break;
        case (BLD_TREE): {
            return tree_next(&iter->as.tree_iter, value_ptr_ptr);
        } break;
        case (BLD_GRAPH): {
            return graph_next(&iter->as.graph_iter, (uintmax_t*) value_ptr_ptr);
        } break;
    }
    log_fatal("iter_next: unreachable error???");
    return 0; /* Unreachable */
}
