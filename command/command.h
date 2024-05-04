#ifndef COMMANDS_H
#define COMMANDS_H
#include "../bld_core/args.h"
#include "invalid.h"
#include "init.h"
#include "remove.h"
#include "build.h"
#include "status.h"
#include "help.h"

typedef enum bld_command_type {
    BLD_COMMAND_INVALID,
    BLD_COMMAND_INIT,
    BLD_COMMAND_REMOVE,
    BLD_COMMAND_BUILD,
    BLD_COMMAND_STATUS,
    BLD_COMMAND_HELP
} bld_command_type;

typedef union bld_union_command {
    bld_command_invalid invalid;
    bld_command_init init;
    bld_command_remove remove;
    bld_command_build build;
    bld_command_status status;
    bld_command_help help;
} bld_union_command;

typedef struct bld_command {
    bld_command_type type;
    bld_union_command as;
} bld_command;

bld_command command_parse(bld_args*, bld_data*);
int command_execute(bld_command*, bld_data*);
void command_free(bld_command*);

#endif
