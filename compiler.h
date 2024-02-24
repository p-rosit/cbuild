#ifndef COMPILER_H
#define COMPILER_H

#include "dstr.h"

typedef struct bld_options {
    size_t capacity;
    size_t size;
    char** options;
} bld_options;

typedef enum bld_compiler_type {
    BLD_GCC,
    BLD_CLANG,
} bld_compiler_type;

typedef struct bld_compiler {
    bld_compiler_type type;
    char* executable;
    bld_options options;
} bld_compiler;

bld_compiler    new_compiler(bld_compiler_type, char*);
bld_compiler    copy_compiler(bld_compiler*);
void            free_compiler(bld_compiler*);
uintmax_t       hash_compiler(bld_compiler*, uintmax_t);
void            add_option(bld_compiler*, char*);

bld_options     new_options();
void            free_options(bld_options*);
void            append_option(bld_options*, char*);

#endif
