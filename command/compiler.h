#ifndef COMMAND_COMPILER_H
#define COMMAND_COMPILER_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_compiler;

typedef struct bld_command_compiler {
    bld_string target;
    int has_path;
    bld_path path;
} bld_command_compiler;

int command_compiler_parse(bld_string*, bld_args*, bld_data*, bld_command_compiler*, bld_command_invalid*);
int command_compiler(bld_command_compiler*, bld_data*);
void command_compiler_free(bld_command_compiler*);

#endif
