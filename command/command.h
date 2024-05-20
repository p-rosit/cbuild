#ifndef COMMANDS_H
#define COMMANDS_H
#include "../bld_core/args.h"
#include "add.h"
#include "build.h"
#include "compiler.h"
#include "help.h"
#include "ignore.h"
#include "init.h"
#include "invalid.h"
#include "invalidate.h"
#include "linker.h"
#include "remove.h"
#include "status.h"
#include "switch.h"

typedef enum bld_command_type {
    BLD_COMMAND_INVALID,

    BLD_COMMAND_INIT,
    BLD_COMMAND_SWITCH,
    BLD_COMMAND_REMOVE,
    BLD_COMMAND_HELP,

    BLD_COMMAND_ADD,
    BLD_COMMAND_BUILD,
    BLD_COMMAND_COMPILER,
    BLD_COMMAND_IGNORE,
    BLD_COMMAND_INVALIDATE,
    BLD_COMMAND_LINKER,
    BLD_COMMAND_STATUS
} bld_command_type;

typedef union bld_union_command {
    bld_command_invalid invalid;
    bld_command_init init;
    bld_command_switch cmd_switch;
    bld_command_remove remove;
    bld_command_help help;
    bld_command_add add;
    bld_command_build build;
    bld_command_compiler compiler;
    bld_command_ignore ignore;
    bld_command_invalidate invalidate;
    bld_command_linker linker;
    bld_command_status status;
} bld_union_command;

typedef struct bld_application_command {
    bld_command_type type;
    bld_union_command as;
} bld_application_command;

bld_set application_available_set(char*);
void application_available_free(bld_set*);
bld_application_command application_command_parse(bld_args*, bld_data*, bld_set*);
int application_command_execute(bld_application_command*, bld_data*, bld_set*);
void application_command_free(bld_application_command*, bld_set*);

#endif
