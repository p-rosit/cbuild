#ifndef COMPILER_H
#define COMPILER_H

#include "dstr.h"
#include "container.h"

typedef enum bld_compiler_type {
    BLD_INVALID_COMPILER,
    BLD_GCC,
    BLD_CLANG,
} bld_compiler_type;

typedef struct bld_compiler {
    bld_compiler_type type;
    char* executable;
    bld_array flags;
} bld_compiler;

bld_compiler    compiler_new(bld_compiler_type, char*);
bld_compiler    compiler_with_flags(bld_compiler_type, char*, ...);
bld_compiler    compiler_copy(bld_compiler*);
void            compiler_free(bld_compiler*);
uintmax_t       compiler_hash(bld_compiler*, uintmax_t);
void            compiler_add_flag(bld_compiler*, char*);

#endif
