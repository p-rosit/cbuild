#ifndef COMMAND_BUILD_H
#define COMMAND_BUILD_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

typedef struct bld_command_build {
    bld_string target;
} bld_command_build;

int command_build_parse(bld_string*, bld_args*, bld_data*, bld_command_build*, bld_command_invalid*);
int command_build(bld_command_build*, bld_data*);
void command_build_free(bld_command_build*);

#endif
