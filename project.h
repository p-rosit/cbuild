#ifndef PROJECT_H
#define PROJECT_H

#include "container.h"
#include "path.h"
#include "compiler.h"
#include "file.h"
#include "graph.h"

#define BLD_CACHE_NAME "cache.json"

typedef struct bld_project bld_project;
struct bld_project {
    bld_path root;
    bld_path build;
    bld_array extra_paths;
    bld_set ignore_paths;
    uintmax_t main_file;
    bld_compiler compiler;
    bld_set files;
    bld_set changed_files;
    bld_graph graph;
    bld_project* cache;
};

bld_project project_new(bld_path, bld_compiler);
void        project_free(bld_project*);

bld_path    project_path_extract(int, char**);
void        project_add_build(bld_project*, char*);
void        project_add_path(bld_project*, char*);
void        project_ignore_path(bld_project*, char*);
void        project_load_cache(bld_project*, char*);
void        project_save_cache(bld_project*);

void        project_set_main_file(bld_project*, char*);
void        project_set_compiler(bld_project*, char*, bld_compiler);

#endif
