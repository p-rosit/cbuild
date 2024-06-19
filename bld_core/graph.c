#include "logging.h"
#include "array.h"
#include "graph.h"
#include "iter.h"

bld_graph graph_new(void) {
    bld_graph graph;
    graph.edges = set_new(sizeof(bld_array));
    return graph;
}

void graph_free(bld_graph* graph) {
    bld_iter iter;
    bld_array* edges;

    iter = iter_set(&graph->edges);
    while (iter_next(&iter, (void**) &edges)) {
        array_free(edges);
    }
    set_free(&graph->edges);
}

void graph_add_node(bld_graph* graph, uintmax_t node_id) {
    bld_array edges;
    edges = array_new(sizeof(bld_hash));
    set_add(&graph->edges, node_id, &edges);
}

void graph_add_edge(bld_graph* graph, uintmax_t from_id, uintmax_t to_id) {
    bld_array* edges;

    edges = set_get(&graph->edges, from_id);
    if (edges == NULL) {log_fatal("From node does not exist.");}
    if (set_get(&graph->edges, to_id) == NULL) {log_fatal("To node does not exist.");}

    array_push(edges, &to_id);
}

int graph_has_node(const bld_graph* graph, uintmax_t node_id) {
    return set_has(&graph->edges, node_id);
}
