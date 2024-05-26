#ifndef COMMAND_COMPILER_H
#define COMMAND_COMPILER_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "handle.h"
#include "invalid.h"

extern const bld_string bld_command_string_compiler;

typedef struct bld_command_compiler {
    bld_string target;
    bld_path path;
    int add_flags;
    bld_array flags;
} bld_command_compiler;

bld_handle_annotated command_handle_compiler(char*);
int command_compiler_convert(bld_command*, bld_data*, bld_command_compiler*, bld_command_invalid*);
int command_compiler(bld_command_compiler*, bld_data*);
void command_compiler_free(bld_command_compiler*);

#endif
