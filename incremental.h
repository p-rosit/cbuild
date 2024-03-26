#ifndef INCREMENTAL_H
#include "project.h"

void    incremental_index_project(bld_project*);
int     incremental_compile_project(bld_project*, char*);
int     incremental_test_project(bld_project*, char*);

#endif
