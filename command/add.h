#ifndef COMMAND_ADD_H
#define COMMAND_ADD_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_add;

typedef struct bld_command_add {
    bld_string target;
    bld_path path;
} bld_command_add;

int command_add_parse(bld_string*, bld_args*, bld_data*, bld_command_add*, bld_command_invalid*);
int command_add(bld_command_add*, bld_data*);
void command_add_free(bld_command_add*);

#endif
