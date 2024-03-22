#include "logging.h"
#include "container.h"

bld_graph graph_new(void) {
    return (bld_graph) {
        .edges = set_new(sizeof(bld_array)),
    };
}

void graph_free(bld_graph* graph) {
    bld_iter iter = iter_set(&graph->edges);
    bld_array* edges;

    while (iter_next(&iter, (void**) &edges)) {
        array_free(edges);
    }
    set_free(&graph->edges);
}

void graph_add_node(bld_graph* graph, uintmax_t node_id) {
    bld_array edges = array_new(sizeof(bld_hash));
    set_add(&graph->edges, node_id, &edges);
}

void graph_add_edge(bld_graph* graph, uintmax_t from_id, uintmax_t to_id) {
    bld_array* edges;

    edges = set_get(&graph->edges, from_id);
    if (edges == NULL) {log_fatal("From node does not exist.");}
    if (set_get(&graph->edges, to_id) == NULL) {log_fatal("To node does not exist.");}

    array_push(edges, &to_id);
}
