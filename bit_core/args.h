#ifndef ARGS_H
#define ARGS_H
#include "dstr.h"

typedef struct bld_args {
    int argc;
    char** argv;
} bld_args;

bld_args    args_new(int, char**);
int         args_empty(bld_args*);
bld_string  args_advance(bld_args*);
bld_string  args_next(bld_args*);

#endif
