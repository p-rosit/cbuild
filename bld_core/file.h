#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <inttypes.h>
#include "set.h"
#include "path.h"
#include "compiler.h"
#include "linker.h"
#include "language/language_types.h"

typedef uintmax_t bld_file_id;
typedef uintmax_t bld_time;

typedef enum bld_file_type {
    BLD_FILE_INVALID,
    BLD_FILE_DIRECTORY,
    BLD_FILE_IMPLEMENTATION,
    BLD_FILE_INTERFACE,
    BLD_FILE_TEST
} bld_file_type;

typedef struct bld_file_build_information {
    int compiler_set;
    int linker_set;
    bld_compiler_or_flags compiler;
    bld_linker_flags linker_flags;
} bld_file_build_information;

typedef struct bld_file_identifier {
    bld_file_id id;
    bld_hash hash;
    bld_time time;
} bld_file_identifier;

typedef struct bld_file_directory {
    bld_array files;
} bld_file_directory;

typedef struct bld_file_implementation {
    bld_set includes;
    bld_set undefined_symbols;
    bld_set defined_symbols;
} bld_file_implementation;

typedef struct bld_file_interface {
    bld_set includes;
} bld_file_interface;

typedef struct bld_file_test {
    bld_set includes;
    bld_set undefined_symbols;
} bld_file_test;

typedef union bld_file_info {
    bld_file_directory dir;
    bld_file_implementation impl;
    bld_file_interface header;
    bld_file_test test;
} bld_file_info;

typedef struct bld_file {
    bld_file_type type;
    bld_language_type language;
    bld_file_id parent_id;
    bld_file_identifier identifier;
    bld_path path;
    bld_string name;
    bld_file_info info;
    bld_file_build_information build_info;
} bld_file;

bld_file    file_directory_new(bld_path*, bld_path*, char*);
bld_file    file_interface_new(bld_path*, bld_path*, char*);
bld_file    file_implementation_new(bld_path*, bld_path*, char*);
bld_file    file_test_new(bld_path*, bld_path*, char*);
void        file_free(bld_file*);
void        file_build_info_free(bld_file_build_information*);

bld_set*    file_includes_get(bld_file*);
bld_set*    file_defined_get(bld_file*);
bld_set*    file_undefined_get(bld_file*);
uintmax_t   file_hash(bld_file*, bld_set*);
int         file_eq(bld_file*, bld_file*);
uintmax_t   file_get_id(bld_path*);
void        file_includes_copy(bld_file*, bld_file*);
void        file_symbols_copy(bld_file*, bld_file*);
bld_string  file_object_name(bld_file*);

void        file_dir_add_file(bld_file*, bld_file*);

void        file_assemble_compiler(bld_file*, bld_set*, bld_compiler**, bld_array*);
void        file_assemble_linker_flags(bld_file*, bld_set*, bld_array*);

#endif
