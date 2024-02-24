#ifndef PROJECT_H
#define PROJECT_H

#include "path.h"
#include "compiler.h"
#include "file.h"
#include "cache.h"
#include "graph.h"

typedef struct bld_extra {
    size_t capacity;
    size_t size;
    bld_path* paths;
} bld_extra;

typedef struct bld_project {
    bld_path root;
    bld_path build;
    bld_paths extra_paths;
    bld_paths ignore_paths;
    bld_file main_file;
    bld_compiler compiler;
    bld_files files;
    bld_graph graph;
    bld_cache* cache;
} bld_project;

bld_project new_project(bld_path, bld_compiler);
void        free_project(bld_project*);
void        print_project(bld_project*);

bld_path    extract_root(int, char**);
void        set_main_file(bld_project*, char*);
void        add_build(bld_project*, char*);
void        add_path(bld_project*, char*);
void        ignore_path(bld_project*, char*);
void        load_cache(bld_project*, char*);
void        save_cache(bld_project*);

void        index_project(bld_project*);
int         compile_project(bld_project*, char*);
int         test_project(bld_project*, char*);

#endif
