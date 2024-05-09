#ifndef COMMAND_SWITCH_H
#define COMMAND_SWITCH_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_switch;

typedef struct bld_command_switch {
    bld_string target;
} bld_command_switch;

int command_switch_parse(bld_args*, bld_data*, bld_command_switch*, bld_command_invalid*);
int command_switch(bld_command_switch*, bld_data*);
void command_switch_free(bld_command_switch*);

#endif
