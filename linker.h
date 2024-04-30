#ifndef LINKER_H
#define LINKER_H

#include <stdio.h>
#include "dstr.h"
#include "array.h"

typedef struct bit_linker_flags {
    bit_array flags;
} bit_linker_flags;

typedef struct bit_linker {
    bit_string executable;
    bit_linker_flags flags;
} bit_linker;

bit_linker          linker_new(char*);
bit_linker          linker_with_flags(char*, ...);
bit_linker          linker_copy(bit_linker*);
void                linker_free(bit_linker*);
uintmax_t           linker_hash(bit_linker*);
void                linker_add_flag(bit_linker*, char*);

bit_linker_flags    linker_flags_new(void);
bit_linker_flags    linker_flags_with_flags(char*, ...);
bit_linker_flags    linker_flags_copy(bit_linker_flags*);
void                linker_flags_free(bit_linker_flags*);
uintmax_t           linker_flags_hash(bit_linker_flags*);
void                linker_flags_add_flag(bit_linker_flags*, char*);

void                linker_flags_expand(bit_string*, bit_array*);
void                linker_flags_append(bit_string*, bit_linker_flags*);

int                 parse_linker(FILE*, bit_linker*);
int                 parse_linker_executable(FILE*, bit_linker*);
int                 parse_linker_linker_flags(FILE*, bit_linker*);
int                 parse_linker_flags(FILE*, bit_linker_flags*);
int                 parse_linker_flag(FILE*, bit_linker_flags*);

#endif
