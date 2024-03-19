#ifndef PROJECT_H
#define PROJECT_H

#include "container.h"
#include "path.h"
#include "compiler.h"
#include "file.h"
#include "graph.h"

#define BLD_CACHE_NAME "cache.json"

typedef struct bld_ignore {
    bld_set set;
} bld_ignore;

typedef struct bld_project bld_project;
struct bld_project {
    bld_path root;
    bld_path build;
    bld_array extra_paths;
    bld_ignore ignore_paths;
    uintmax_t main_file;
    bld_compiler compiler;
    bld_set files;
    bld_set changed_files;
    bld_graph graph;
    bld_project* cache;
};

bld_project new_project(bld_path, bld_compiler);
void        free_project(bld_project*);

bld_path    extract_path(int, char**);
void        add_build(bld_project*, char*);
void        add_path(bld_project*, char*);
void        add_absolute(bld_project*, char*);
void        ignore_path(bld_project*, char*);
void        load_cache(bld_project*, char*);
void        save_cache(bld_project*);

void        set_main_file(bld_project*, char*);
void        set_compiler(bld_project*, char*, bld_compiler);

#endif
