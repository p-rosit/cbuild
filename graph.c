#include "logging.h"
#include "array.h"
#include "graph.h"
#include "iter.h"

bit_graph graph_new(void) {
    bit_graph graph;
    graph.edges = set_new(sizeof(bit_array));
    return graph;
}

void graph_free(bit_graph* graph) {
    bit_iter iter = iter_set(&graph->edges);
    bit_array* edges;

    while (iter_next(&iter, (void**) &edges)) {
        array_free(edges);
    }
    set_free(&graph->edges);
}

void graph_add_node(bit_graph* graph, uintmax_t node_id) {
    bit_array edges = array_new(sizeof(bit_hash));
    set_add(&graph->edges, node_id, &edges);
}

void graph_add_edge(bit_graph* graph, uintmax_t from_id, uintmax_t to_id) {
    bit_array* edges;

    edges = set_get(&graph->edges, from_id);
    if (edges == NULL) {log_fatal("From node does not exist.");}
    if (set_get(&graph->edges, to_id) == NULL) {log_fatal("To node does not exist.");}

    array_push(edges, &to_id);
}

int graph_has_node(const bit_graph* graph, uintmax_t node_id) {
    return set_has(&graph->edges, node_id);
}
