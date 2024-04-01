#ifndef COMPILER_H
#define COMPILER_H

#include "dstr.h"
#include "container.h"

typedef struct bld_compiler {
    char* executable;
    bld_array flags;
} bld_compiler;

bld_compiler    compiler_new(char*);
bld_compiler    compiler_with_flags(char*, ...);
bld_compiler    compiler_copy(bld_compiler*);
void            compiler_free(bld_compiler*);
uintmax_t       compiler_hash(bld_compiler*, uintmax_t);
void            compiler_add_flag(bld_compiler*, char*);

#endif
