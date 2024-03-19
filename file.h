#ifndef FILE_H
#define FILE_H

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include <inttypes.h>
#include "path.h"
#include "compiler.h"

typedef enum bld_file_type {
    BLD_INVALID_FILE,
    BLD_IMPL,
    BLD_HEADER,
    BLD_TEST,
} bld_file_type;

typedef struct bld_file_identifier {
    uintmax_t id;
    uintmax_t hash;
    uintmax_t time;
} bld_file_identifier;

typedef struct bld_file {
    bld_file_type type;
    bld_file_identifier identifier;
    bld_path path;
    bld_string name;
    bld_compiler* compiler;
    bld_set defined_symbols;
    bld_set undefined_symbols;
    bld_set includes;
} bld_file;

typedef struct bld_files {
    bld_set set;
} bld_files;

bld_file    file_header_new(bld_path*, char*);
bld_file    file_impl_new(bld_path*, char*);
bld_file    file_test_new(bld_path*, char*);
void        file_free(bld_file*);
uintmax_t   hash_file(bld_file*, uintmax_t);
int         file_eq(bld_file*, bld_file*);
uintmax_t   get_file_id(bld_path*);
void        serialize_identifier(char[FILENAME_MAX], bld_file*);

bld_files   new_files();
void        clear_files(bld_files*);
void        free_files(bld_files*);
int         append_file(bld_files*, bld_file);

#endif
