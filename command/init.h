#ifndef COMMAND_INIT_H
#define COMMAND_INIT_H
#include "../bit_core/dstr.h"
#include "../bit_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_init;

typedef struct bld_command_init {
    int init_project;
    bld_string target;
} bld_command_init;

int command_init_parse(bld_args*, bld_command_init*, bld_command_invalid*);
int command_init(bld_command_init*, bld_data*);
void command_init_free(bld_command_init*);

#endif
