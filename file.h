#ifndef FILE_H
#define FILE_H

#include <dirent.h>
#include "path.h"
#include "compiler.h"

typedef struct dirent bld_dirent;

typedef enum bld_file_type {
    BLD_INVALID,
    BLD_IMPL,
    BLD_HEADER,
    BLD_TEST,
} bld_file_type;

typedef struct bld_file {
    bld_file_type type;
    bld_path path;
    bld_string name;
    bld_compiler* compiler;
} bld_file;

typedef struct bld_files {
    size_t capacity;
    size_t size;
    bld_file* files;
} bld_files;

bld_file    make_header(bld_path*, bld_dirent*);
bld_file    make_impl(bld_path*, bld_dirent*);
bld_file    make_test(bld_path*, bld_dirent*);
void        free_file(bld_file*);

bld_files   new_files();
void        clear_files(bld_files*);
void        free_files(bld_files*);
void        append_file(bld_files*, bld_file);

#endif
