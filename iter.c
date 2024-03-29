#include "logging.h"
#include "container.h"

bld_iter iter_array(const bld_array* array) {
    return (bld_iter) {
        .type = BLD_ARRAY,
        .as = (union bld_iter_container) {
            .array_iter = (bld_iter_array) {
                .array = array,
                .index = 0,
            },
        },
    };
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
    return (bld_iter) {
        .type = BLD_SET,
        .as = (union bld_iter_container) {
            .set_iter = (bld_iter_set) {
                .set = set,
                .index = 0,
            },
        },
    };
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

bld_iter iter_graph(const bld_graph* graph, uintmax_t root) {
    bld_iter iter = (bld_iter) {
        .type = BLD_GRAPH,
        .as = (union bld_iter_container) {
            .graph_iter = (bld_iter_graph) {
                .type = BLD_DFS,
                .graph = graph,
                .stack = array_new(sizeof(uintmax_t)),
                .visited = set_new(0),
            },
        },
    };

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
        bld_iter edges = iter_array(edge_array);
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
        case (BLD_GRAPH): {
            return graph_next(&iter->as.graph_iter, (uintmax_t*) value_ptr_ptr);
        } break;
    }
    log_fatal("iter_next: unreachable error???");
    return 0; /* Unreachable */
}
