#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <inttypes.h>
#include "set.h"
#include "path.h"
#include "compiler.h"
#include "linker.h"

typedef enum bld_file_type {
    BLD_INVALID_FILE,
    BLD_DIR,
    BLD_IMPL,
    BLD_HEADER,
    BLD_TEST
} bld_file_type;

typedef struct bld_file_build_information {
    int compiler_set;
    int linker_set;
    bld_compiler_or_flags compiler;
    bld_linker_flags linker_flags;
} bld_file_build_information;

typedef struct bld_file_identifier {
    uintmax_t id;
    uintmax_t hash;
    uintmax_t time;
} bld_file_identifier;

typedef struct bld_file_dir {
    bld_array files;
} bld_file_dir;

typedef struct bld_file_impl {
    bld_set includes;
    bld_set undefined_symbols;
    bld_set defined_symbols;
} bld_file_impl;

typedef struct bld_file_header {
    bld_set includes;
} bld_file_header;

typedef struct bld_file_test {
    bld_set includes;
    bld_set undefined_symbols;
} bld_file_test;

typedef union bld_file_info {
    bld_file_dir dir;
    bld_file_impl impl;
    bld_file_header header;
    bld_file_test test;
} bld_file_info;

typedef struct bld_file {
    bld_file_type type;
    uintmax_t parent_id;
    bld_file_identifier identifier;
    bld_path path;
    bld_string name;
    bld_file_info info;
    bld_file_build_information build_info;
} bld_file;

bld_file    file_dir_new(bld_path*, char*);
bld_file    file_header_new(bld_path*, char*);
bld_file    file_impl_new(bld_path*, char*);
bld_file    file_test_new(bld_path*, char*);
void        file_free(bld_file*);
void        file_build_info_free(bld_file_build_information*);

uintmax_t   file_hash(bld_file*, bld_set*);
int         file_eq(bld_file*, bld_file*);
uintmax_t   file_get_id(bld_path*);
void        file_symbols_copy(bld_file*, const bld_file*);
void        serialize_identifier(char[FILENAME_MAX], bld_file*);

void        file_dir_add_file(bld_file*, bld_file*);

void        file_assemble_compiler(bld_file*, bld_set*, bld_compiler**, bld_array*);
void        file_assemble_linker_flags(bld_file*, bld_set*, bld_array*);

#endif
