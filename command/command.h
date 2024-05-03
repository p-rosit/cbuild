#ifndef COMMANDS_H
#define COMMANDS_H
#include "../bit_core/args.h"
#include "invalid.h"
#include "init.h"
#include "remove.h"

typedef enum bld_command_type {
    BLD_COMMAND_INVALID,
    BLD_COMMAND_INIT,
    BLD_COMMAND_REMOVE
} bld_command_type;

typedef union bld_union_command {
    bld_command_invalid invalid;
    bld_command_init init;
    bld_command_remove remove;
} bld_union_command;

typedef struct bld_command {
    bld_command_type type;
    bld_union_command as;
} bld_command;

bld_command command_parse(bld_args*);
int command_execute(bld_command*, bld_data*);
void command_free(bld_command*);

#endif
