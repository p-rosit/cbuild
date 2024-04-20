#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include "dstr.h"
#include "array.h"

typedef struct bld_compiler_flags {
    bld_array flags;
} bld_compiler_flags;

typedef struct bld_compiler {
    bld_string executable;
    bld_compiler_flags flags;
} bld_compiler;

enum bld_compiler_or_flags_type {
    BLD_COMPILER,
    BLD_COMPILER_FLAGS
};

union bld_compiler_and_flags {
    bld_compiler compiler;
    bld_compiler_flags flags;
};

typedef struct bld_compiler_or_flags {
    enum bld_compiler_or_flags_type type;
    union bld_compiler_and_flags as;
} bld_compiler_or_flags;

bld_compiler        compiler_new(char*);
bld_compiler        compiler_with_flags(char*, ...);
bld_compiler        compiler_copy(bld_compiler*);
void                compiler_free(bld_compiler*);
uintmax_t           compiler_hash(bld_compiler*);
void                compiler_add_flag(bld_compiler*, char*);

bld_compiler_flags  compiler_flags_new(void);
bld_compiler_flags  compiler_flags_with_flags(char*, ...);
bld_compiler_flags  compiler_flags_copy(bld_compiler_flags*);
void                compiler_flags_free(bld_compiler_flags*);
uintmax_t           compiler_flags_hash(bld_compiler_flags*);
void                compiler_flags_add_flag(bld_compiler_flags*, char*);

void                compiler_flags_collect(bld_string*, bld_array*);

int                 parse_compiler(FILE*, bld_compiler_or_flags*);
int                 parse_compiler_type(FILE*, bld_compiler*);
int                 parse_compiler_executable(FILE*, bld_compiler*);
int                 parse_compiler_flags(FILE*, bld_compiler*);
int                 parse_compiler_flag(FILE*, bld_compiler_flags*);

#endif
