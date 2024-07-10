#ifndef PROJECT_BASE_H
#define PROJECT_BASE_H

#include "path.h"
#include "set.h"
#include "linker.h"

typedef struct bld_project_cache bld_project_cache;
typedef struct bld_cache_handle bld_cache_handle;
typedef struct bld_project_base bld_project_base;
typedef struct bld_forward_project bld_forward_project;
typedef struct bld_project bld_project;

struct bld_project_cache {
    int loaded;
    int set;
    int applied;
    bld_project_base* base;
    bld_path root;
    uintmax_t root_file;
    bld_linker linker;
    bld_set files;
};

struct bld_cache_handle {
    int loaded;
    bld_project_base* base;
    bld_path root;
    bld_set files;
    bld_set loaded_files;
};

struct bld_project_base {
    int rebuilding;
    bld_project_base* build_of;
    bld_path root;
    int standalone;
    bld_path build;
    bld_set compiler_handles;
    bld_linker linker;
    bld_project_cache cache;
    bld_cache_handle cache_;
};

#endif
