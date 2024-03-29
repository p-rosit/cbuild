#ifndef PROJECT_H
#define PROJECT_H

#include "container.h"
#include "path.h"
#include "compiler.h"
#include "file.h"
#include "dependencies.h"

#define BLD_CACHE_NAME "cache.json"

typedef struct bld_project_cache {
    int loaded;
    int set;
    int applied;
    bld_path root;
    bld_compiler compiler;
    bld_array file_compilers;
    bld_set files;
} bld_project_cache;

typedef struct bld_project_base {
    bld_path root;
    bld_path build;
    bld_compiler compiler;
    bld_array file_compilers;
    bld_project_cache cache;
} bld_project_base;

typedef struct bld_forward_project {
    int rebuilding;
    bld_project_base base;
    bld_array extra_paths;
    bld_set ignore_paths;
    bld_array file_names;
} bld_forward_project;

typedef struct bld_project {
    bld_project_base base;
    uintmax_t main_file;
    bld_set files;
    bld_dependency_graph graph;
} bld_project;

bld_path    project_path_extract(int, char**);
bld_forward_project project_new(bld_path, bld_compiler);

void        project_add_build(bld_forward_project*, char*);
void        project_add_path(bld_forward_project*, char*);
void        project_ignore_path(bld_forward_project*, char*);
void        project_load_cache(bld_forward_project*, char*);
void        project_set_main_file(bld_forward_project*, char*);
void        project_set_compiler(bld_forward_project*, char*, bld_compiler);

bld_project project_resolve(bld_forward_project*);
void        project_save_cache(bld_project*);
void        project_free(bld_project*);

#endif
