#ifndef INCREMENTAL_H
#include "project.h"

bld_project project_resolve(bld_forward_project*);

void    incremental_apply_cache(bld_project*);
int     incremental_project_dependencies(bld_project*);
int     incremental_compile_executable(bld_project*, char*);
int     incremental_link_executable(bld_project*, char*);

#endif
