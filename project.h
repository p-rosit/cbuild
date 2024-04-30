#ifndef PROJECT_H
#define PROJECT_H

#include "set.h"
#include "path.h"
#include "compiler.h"
#include "linker.h"
#include "file.h"
#include "dependencies.h"

#define BIT_CACHE_NAME "cache.json"

typedef struct bit_project_cache {
    int loaded;
    int set;
    int applied;
    bit_path root;
    bit_linker linker;
    bit_set files;
} bit_project_cache;

typedef struct bit_project_base {
    bit_path root;
    bit_path build;
    bit_linker linker;
    bit_project_cache cache;
} bit_project_base;

typedef struct bit_forward_project {
    int rebuilding;
    int resolved;
    bit_project_base base;
    bit_array extra_paths;
    bit_set ignore_paths;
    bit_compiler compiler;
    bit_string main_file_name;
    bit_array compiler_file_names;
    bit_array linker_flags_file_names;
    bit_array file_compilers;
    bit_array file_linker_flags;
} bit_forward_project;

typedef struct bit_project {
    bit_project_base base;
    uintmax_t main_file;
    uintmax_t root_dir;
    bit_set files;
    bit_dependency_graph graph;
} bit_project;

bit_path    project_path_extract(int, char**);
bit_forward_project project_new(bit_path, bit_compiler, bit_linker);

void        project_add_build(bit_forward_project*, char*);
void        project_add_path(bit_forward_project*, char*);
void        project_ignore_path(bit_forward_project*, char*);
void        project_load_cache(bit_forward_project*, char*);
void        project_set_main_file(bit_forward_project*, char*);
void        project_set_compiler(bit_forward_project*, char*, bit_compiler);
void        project_set_linker_flags(bit_forward_project*, char*, bit_linker_flags);

void        project_save_cache(bit_project*);
void        project_free(bit_project*);
void        project_partial_free(bit_forward_project*);

#endif
