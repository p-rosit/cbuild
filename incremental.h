#ifndef INCREMENTAL_H
#include "project.h"

void        index_project(bld_project*);
void        generate_dependency_graph(bld_project*);
int         compile_project(bld_project*, char*);
int         test_project(bld_project*, char*);

#endif
