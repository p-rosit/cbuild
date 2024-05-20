#ifndef COMMANDS_H
#define COMMANDS_H
#include "../bld_core/args.h"
#include "utils.h"
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

bld_application_command application_command_parse(bld_args*, bld_data*);
int application_command_execute(bld_application_command*, bld_data*);
void application_command_free(bld_application_command*, bld_data*);

#endif
