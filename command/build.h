#ifndef COMMAND_BUILD_H
#define COMMAND_BUILD_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

typedef struct bld_command_build {
    bld_string target;
} bld_command_build;

bld_handle_annotated command_handle_build(char*);
int command_build_convert(bld_command* ,bld_data*, bld_command_build*, bld_command_invalid*);
int command_build(bld_command_build*, bld_data*);
void command_build_free(bld_command_build*);

#endif
