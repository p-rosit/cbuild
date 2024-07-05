#include <assert.h>
#include "../iter.h"
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

void test_iter_array(void) {
    bld_array array;
    int number[] = {3, 4, 2};

    array = array_new(sizeof(int));
    array_push(&array, &number[0]);
    array_push(&array, &number[1]);
    array_push(&array, &number[2]);

    {
        bld_iter iter;
        int* result;
        int index;

        index = 0;
        iter = iter_array(&array);
        while (iter_next(&iter, (void**) &result)) {
            assert(*result == number[index]);
            index += 1;
        }
    }

    array_free(&array);
}

void test_iter_set(void) {
    bld_set set;
    bld_hash hash[] = {0, 1, 2};
    int number[] = {3, 4, 6};

    set = set_new(sizeof(int));
    set_add(&set, hash[0], &number[0]);
    set_add(&set, hash[1], &number[1]);
    set_add(&set, hash[2], &number[2]);

    {
        bld_iter iter;
        unsigned int index;
        int* result;
        int visited[ARRAY_SIZE(number)] = {0};

        iter = iter_set(&set);
        while (iter_next(&iter, (void**) &result)) {
            int match;

            match = 0;
            for (index = 0; index < ARRAY_SIZE(number); index++) {
                if (*result == number[index]) {
                    match = 1;
                    break;
                }
            }

            assert(match);
            assert(!visited[index]);
            visited[index] = 1;
        }

        for (index = 0; index < ARRAY_SIZE(number); index++) {
            assert(visited[index]);
        }
    }

    set_free(&set);
}

void test_iter_graph(void) {
    bld_graph graph;
    uintmax_t nodes[] = {0, 1, 2, 3, 4};
    uintmax_t edges[][2] = {{0, 1}, {0, 2}, {1, 2}, {3, 4}, {4, 3}};
    
    /* 
     * Test graph:
     *
     * 0 ----------> 1
     * |             |
     * |             |
     * -----> 2 <-----
     *
     *
     * 3 <---------> 4
     */

    graph = graph_new();
    {
        unsigned int i;

        for (i = 0; i < ARRAY_SIZE(nodes); i++) {
            graph_add_node(&graph, nodes[i]);
        }

        for (i = 0; i < ARRAY_SIZE(edges); i++) {
            graph_add_edge(&graph, edges[i][0], edges[i][1]);
        }
    }

    {
        bld_iter iter;
        unsigned int index;
        uintmax_t node;
        uintmax_t expected[] = {0, 1, 2};
        int visited[ARRAY_SIZE(expected)] = {0};

        iter = iter_graph(&graph, 0);
        while (iter_next(&iter, (void**) &node)) {
            int match;

            match = 0;
            for (index = 0; index < ARRAY_SIZE(expected); index++) {
                if (node == expected[index]) {
                    match = 1;
                    break;
                }
            }

            assert(match);
            assert(!visited[index]);
            visited[index] = 1;
        }

        for (index = 0; index < ARRAY_SIZE(expected); index++) {
            assert(visited[index]);
        }
    }

    graph_free(&graph);
}

void test_iter_graph_children(void) {
    bld_graph graph;
    uintmax_t nodes[] = {0, 1, 2, 3, 4};
    uintmax_t edges[][2] = {{0, 1}, {0, 2}, {1, 2}, {3, 4}, {4, 3}};
    
    /* 
     * Test graph:
     *
     * 0 ----------> 1
     * |             |
     * |             |
     * -----> 2 <-----
     *
     *
     * 3 <---------> 4
     */

    graph = graph_new();
    {
        unsigned int i;

        for (i = 0; i < ARRAY_SIZE(nodes); i++) {
            graph_add_node(&graph, nodes[i]);
        }

        for (i = 0; i < ARRAY_SIZE(edges); i++) {
            graph_add_edge(&graph, edges[i][0], edges[i][1]);
        }
    }

    {
        bld_iter iter;
        unsigned int index;
        uintmax_t* node;
        uintmax_t expected[] = {1, 2};
        int visited[ARRAY_SIZE(expected)] = {0};

        iter = iter_graph_children(&graph, 0);
        while (iter_next(&iter, (void**) &node)) {
            int match;

            match = 0;
            for (index = 0; index < ARRAY_SIZE(expected); index++) {
                if (*node == expected[index]) {
                    match = 1;
                    break;
                }
            }

            assert(match);
            assert(!visited[index]);
            visited[index] = 1;
        }

        for (index = 0; index < ARRAY_SIZE(expected); index++) {
            assert(visited[index]);
        }
    }

    graph_free(&graph);
}

int main() {
    test_iter_array();
    test_iter_set();
    test_iter_graph();
    test_iter_graph_children();
    return 0;
}
