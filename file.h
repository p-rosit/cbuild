#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <inttypes.h>
#include "set.h"
#include "path.h"
#include "compiler.h"
#include "linker.h"

typedef enum bit_file_type {
    BIT_INVALID_FILE,
    BIT_DIR,
    BIT_IMPL,
    BIT_HEADER,
    BIT_TEST
} bit_file_type;

typedef struct bit_file_build_information {
    int compiler_set;
    int linker_set;
    bit_compiler_or_flags compiler;
    bit_linker_flags linker_flags;
} bit_file_build_information;

typedef struct bit_file_identifier {
    uintmax_t id;
    uintmax_t hash;
    uintmax_t time;
} bit_file_identifier;

typedef struct bit_file_dir {
    bit_array files;
} bit_file_dir;

typedef struct bit_file_impl {
    bit_set includes;
    bit_set undefined_symbols;
    bit_set defined_symbols;
} bit_file_impl;

typedef struct bit_file_header {
    bit_set includes;
} bit_file_header;

typedef struct bit_file_test {
    bit_set includes;
    bit_set undefined_symbols;
} bit_file_test;

typedef union bit_file_info {
    bit_file_dir dir;
    bit_file_impl impl;
    bit_file_header header;
    bit_file_test test;
} bit_file_info;

typedef struct bit_file {
    bit_file_type type;
    uintmax_t parent_id;
    bit_file_identifier identifier;
    bit_path path;
    bit_string name;
    bit_file_info info;
    bit_file_build_information build_info;
} bit_file;

bit_file    file_dir_new(bit_path*, char*);
bit_file    file_header_new(bit_path*, char*);
bit_file    file_impl_new(bit_path*, char*);
bit_file    file_test_new(bit_path*, char*);
void        file_free(bit_file*);

uintmax_t   file_hash(bit_file*, bit_set*);
int         file_eq(bit_file*, bit_file*);
uintmax_t   file_get_id(bit_path*);
void        file_symbols_copy(bit_file*, const bit_file*);
void        serialize_identifier(char[FILENAME_MAX], bit_file*);

void        file_dir_add_file(bit_file*, bit_file*);

void        file_assemble_compiler(bit_file*, bit_set*, bit_string**, bit_array*);
void        file_assemble_linker_flags(bit_file*, bit_set*, bit_array*);

#endif
