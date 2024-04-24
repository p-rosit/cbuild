#ifndef PROJECT_H
#define PROJECT_H

#include "set.h"
#include "path.h"
#include "compiler.h"
#include "linker.h"
#include "file.h"
#include "file_tree.h"
#include "dependencies.h"

#define BLD_CACHE_NAME "cache.json"

typedef struct bld_project_cache {
    int loaded;
    int set;
    int applied;
    bld_path root;
    bld_compiler compiler;
    bld_linker linker;
    bld_set file2compiler;
    bld_array file_compilers;
    bld_set file2linker_flags;
    bld_array file_linker_flags;
    bld_set files;
} bld_project_cache;

typedef struct bld_project_base {
    bld_path root;
    bld_path build;
    bld_compiler compiler;
    bld_linker linker;
    bld_array file_compilers;
    bld_array file_linker_flags;
    bld_project_cache cache;
} bld_project_base;

typedef struct bld_forward_project {
    int rebuilding;
    int resolved;
    bld_project_base base;
    bld_array extra_paths;
    bld_set ignore_paths;
    bld_string main_file_name;
    bld_array compiler_file_names;
    bld_array linker_flags_file_names;
} bld_forward_project;

typedef struct bld_project {
    bld_project_base base;
    uintmax_t main_file;
    uintmax_t root_dir;
    bld_set files;
    bld_dependency_graph graph;
    bld_set file2compiler;
    bld_set file2linker_flags;
} bld_project;

bld_path    project_path_extract(int, char**);
bld_forward_project project_new(bld_path, bld_compiler, bld_linker);

void        project_add_build(bld_forward_project*, char*);
void        project_add_path(bld_forward_project*, char*);
void        project_ignore_path(bld_forward_project*, char*);
void        project_load_cache(bld_forward_project*, char*);
void        project_set_main_file(bld_forward_project*, char*);
void        project_set_compiler(bld_forward_project*, char*, bld_compiler);
void        project_set_linker_flags(bld_forward_project*, char*, bld_linker_flags);

void        project_save_cache(bld_project*);
void        project_free(bld_project*);
void        project_partial_free(bld_forward_project*);

#endif
