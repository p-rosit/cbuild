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
    const bld_array* array;
    size_t value_size;
    char* values;

    array = iter->array;
    values = array->values;
    value_size = array->value_size;
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
    int has_next;
    size_t i;
    size_t value_size;
    const bld_set* set;
    char* values;
    
    has_next = 0;
    set = iter->set;
    values = set->values;
    value_size = set->value_size;

    i = iter->index;
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
    bld_iter iter;

    iter.type = BLD_GRAPH;
    iter.as.graph_iter.type = BLD_DFS;
    iter.as.graph_iter.graph = graph;
    iter.as.graph_iter.stack = array_new(sizeof(uintmax_t));
    iter.as.graph_iter.visited = set_new(0);

    if (!set_has(&graph->edges, root)) {log_fatal("root does not exist");}
    array_push(&iter.as.graph_iter.stack, &root);

    return iter;
}

bld_iter iter_graph_children(const bld_graph* graph, uintmax_t parent) {
    bld_array* children;

    children = set_get(&graph->edges, parent);
    if (children == NULL) {
        log_fatal("Requested node %lu does not exist in graph", parent);
    }

    return iter_array(children);
}

int graph_next(bld_iter_graph* iter, uintmax_t* node_id) {
    int node_visited;
    uintmax_t *ptr_id, id;
    bld_array* edge_array;

    if (iter->type == BLD_GRAPH_DONE) {
        return 0;
    }

    node_visited = 1;
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

bld_iter iter_directory(bld_os_dir* dir) {
    bld_iter iter;
    if (dir == NULL) {log_fatal("iter_directory: directory to traverse is null");}

    iter.type = BLD_DIRECTORY;
    iter.as.directory_iter.dir = dir;
    return iter;
}

int directory_next(bld_iter_directory* iter, bld_string* name_ptr) {
    bld_os_file* file;
    file = os_dir_read(iter->dir);

    if (file != NULL) {
        *name_ptr = string_pack(os_file_name(file));
        return 1;
    } else {
        return 0;
    }
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
        case (BLD_DIRECTORY): {
            return directory_next(&iter->as.directory_iter, (bld_string*) value_ptr_ptr);
        } break;
    }
    log_fatal("iter_next: unreachable error???");
    return 0; /* Unreachable */
}
