#include <assert.h>
#include "../array.h"
#include "../graph.h"

void test_graph_new(void) {
    bld_graph graph;
    bld_set* edges;

    graph = graph_new();
    edges = &graph.edges;

    assert(edges->capacity == 0);
    assert(edges->size == 0);
    assert(edges->max_offset == 0);
    assert(edges->value_size >= sizeof(size_t));

    assert(edges->values == NULL);
    assert(edges->hash == NULL);
    assert(edges->offset == NULL);
}

void test_graph_free(void) {
    bld_graph graph;

    graph = graph_new();
    graph_free(&graph);
}

void test_graph_add_node(void) {
    bld_graph graph;
    uintmax_t node_id;

    graph = graph_new();

    node_id = 0;
    graph_add_node(&graph, node_id);

    {
        bld_array* node;

        node = set_get(&graph.edges, node_id);
        assert(node != NULL);
        assert(node->size == 0);
    }

    graph_free(&graph);
}

void test_graph_add_edge(void) {
    bld_graph graph;
    uintmax_t node_id[] = {3, 4};

    graph = graph_new();

    graph_add_node(&graph, node_id[0]);
    graph_add_node(&graph, node_id[1]);
    graph_add_edge(&graph, node_id[0], node_id[1]);

    {
        bld_array* from_edge;
        bld_array* to_edge;
        uintmax_t* to_node;

        from_edge = set_get(&graph.edges, node_id[0]);
        assert(from_edge != NULL);

        to_edge = set_get(&graph.edges, node_id[1]);
        assert(to_edge != NULL);

        assert(from_edge->size == 1);
        assert(to_edge->size == 0);

        to_node = array_get(from_edge, 0);
        assert(*to_node == node_id[1]);
    }

    graph_free(&graph);
}

void test_graph_has_node(void) {
    bld_graph graph;
    uintmax_t node_id;

    graph = graph_new();

    node_id = 0;
    graph_add_node(&graph, node_id);

    assert(graph_has_node(&graph, node_id));
    assert(!graph_has_node(&graph, node_id + 1));

    graph_free(&graph);
}

int main() {
    test_graph_new();
    test_graph_free();
    test_graph_add_node();
    test_graph_add_edge();
    test_graph_has_node();
    return 0;
}
