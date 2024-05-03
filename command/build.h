#ifndef COMMAND_BUILD_H
#define COMMAND_BUILD_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

typedef enum bld_command_build_sub_command {
    BLD_COMMAND_BUILD_TARGET,
    BLD_COMMAND_BUILD_COMPILER,
    BLD_COMMAND_BUILD_LINKER
} bld_command_build_sub_command;

typedef struct bld_command_build {
    bld_command_build_sub_command type;
    bld_string target;
    bld_path path;
} bld_command_build;

int command_build_parse(bld_string*, bld_args*, bld_data*, bld_command_build*, bld_command_invalid*);
int command_build(bld_command_build*, bld_data*);
void command_build_free(bld_command_build*);

#endif
