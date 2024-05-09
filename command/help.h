#ifndef COMMAND_HELP_H
#define COMMAND_HELP_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_help;

typedef struct bld_command_help {
    int has_command;
    bld_string command;
} bld_command_help;

int command_help_parse(bld_args*, bld_data*, bld_command_help*, bld_command_invalid*);
int command_help(bld_command_help*, bld_data*);
void command_help_free(bld_command_help*);

#endif
