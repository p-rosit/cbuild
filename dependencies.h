#ifndef GRAPH_H
#define GRAPH_H

#include "container.h"
#include "path.h"
#include "file.h"

typedef struct bld_dependency_graph {
    bld_set* files;
    bld_graph include_graph;
    bld_graph symbol_graph;
} bld_dependency_graph;

bld_dependency_graph dependency_graph_new(bld_set*);
void        dependency_graph_free(bld_dependency_graph*);

void        dependency_graph_extract_includes(bld_dependency_graph*);
void        dependency_graph_extract_symbols(bld_dependency_graph*, bld_path*);

bld_iter    dependency_graph_symbols_from(const bld_dependency_graph*, bld_file*);
bld_iter    dependency_graph_includes_from(const bld_dependency_graph*, bld_file*);
int         dependency_graph_next_file(bld_iter*, const bld_dependency_graph*, bld_file**);

#endif
