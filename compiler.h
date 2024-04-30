#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include "dstr.h"
#include "array.h"
#include "set.h"

typedef struct bit_compiler_flags {
    bit_array flags;
    bit_set flag_hash;
    bit_set removed;
} bit_compiler_flags;

typedef struct bit_compiler {
    bit_string executable;
    bit_compiler_flags flags;
} bit_compiler;

enum bit_compiler_or_flags_type {
    BIT_COMPILER,
    BIT_COMPILER_FLAGS
};

union bit_compiler_and_flags {
    bit_compiler compiler;
    bit_compiler_flags flags;
};

typedef struct bit_compiler_or_flags {
    enum bit_compiler_or_flags_type type;
    union bit_compiler_and_flags as;
} bit_compiler_or_flags;

bit_compiler        compiler_new(char*);
bit_compiler        compiler_with_flags(char*, ...);
bit_compiler        compiler_copy(bit_compiler*);
void                compiler_free(bit_compiler*);
uintmax_t           compiler_hash(bit_compiler*);
void                compiler_add_flag(bit_compiler*, char*);
void                compiler_remove_flag(bit_compiler*, char*);

bit_compiler_flags  compiler_flags_new(void);
bit_compiler_flags  compiler_flags_with_flags(char*, ...);
bit_compiler_flags  compiler_flags_copy(bit_compiler_flags*);
void                compiler_flags_free(bit_compiler_flags*);
uintmax_t           compiler_flags_hash(bit_compiler_flags*);
void                compiler_flags_add_flag(bit_compiler_flags*, char*);
void                compiler_flags_remove_flag(bit_compiler_flags*, char*);

void                compiler_flags_expand(bit_string*, bit_array*);

int                 parse_compiler(FILE*, bit_compiler*);
int                 parse_compiler_executable(FILE*, bit_compiler*);
int                 parse_compiler_compiler_flags(FILE*, bit_compiler*);

int                 parse_compiler_flags(FILE*, bit_compiler_flags*);
int                 parse_compiler_flags_added_flags(FILE*, bit_compiler_flags*);
int                 parse_compiler_flags_added_flag(FILE*, bit_compiler_flags*);
int                 parse_compiler_flags_removed_flags(FILE*, bit_compiler_flags*);
int                 parse_compiler_flags_removed_flag(FILE*, bit_compiler_flags*);

#endif
