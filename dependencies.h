#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "set.h"
#include "graph.h"
#include "iter.h"
#include "path.h"
#include "file.h"

typedef struct bit_dependency_graph {
    bit_graph include_graph;
    bit_graph symbol_graph;
} bit_dependency_graph;

bit_dependency_graph dependency_graph_new(void);
void        dependency_graph_free(bit_dependency_graph*);

void        dependency_graph_extract_includes(bit_dependency_graph*, bit_set*);
void        dependency_graph_extract_symbols(bit_dependency_graph*, bit_set*, bit_path*);

bit_iter    dependency_graph_symbols_from(const bit_dependency_graph*, bit_file*);
bit_iter    dependency_graph_includes_from(const bit_dependency_graph*, bit_file*);
int         dependency_graph_next_file(bit_iter*, const bit_set*, bit_file**);

#endif
