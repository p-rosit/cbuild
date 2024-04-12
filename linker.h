#ifndef LINKER_H

#include <stdio.h>
#include "dstr.h"
#include "array.h"

typedef struct bld_linker_flags {
    bld_array flag;
    bld_array hash;
} bld_linker_flags;

typedef struct bld_linker {
    bld_string executable;
    bld_linker_flags flags;
} bld_linker;

bld_linker          linker_new(char*);
bld_linker          linker_with_flags(char*, ...);
bld_linker          linker_copy(bld_linker*);
void                linker_free(bld_linker*);
void                linker_add_flag(bld_linker*, char*);

bld_linker_flags    linker_flags_new(void);
bld_linker_flags    linker_flags_with_flags(char*, ...);
bld_linker_flags    linker_flags_copy(bld_linker_flags*);
void                linker_flags_free(bld_linker_flags*);
void                linker_flags_add_flag(bld_linker_flags*, char*);

void                linker_flags_collect(bld_string*, bld_array*);

int                 parse_linker(FILE*, bld_linker*);
int                 parse_linker_executable(FILE*, bld_linker*);
int                 parse_linker_linker_flags(FILE*, bld_linker*);
int                 parse_linker_flags(FILE*, bld_linker_flags*);
int                 parse_linker_flag(FILE*, bld_linker_flags*);

#endif
