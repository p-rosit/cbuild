#ifndef INCREMENTAL_H
#include "project.h"

bit_project project_resolve(bit_forward_project*);

void    incremental_apply_cache(bit_project*);
int     incremental_compile_project(bit_project*, char*);
int     incremental_test_project(bit_project*, char*);

#endif
