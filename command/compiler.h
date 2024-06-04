#ifndef COMMAND_COMPILER_H
#define COMMAND_COMPILER_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "handle.h"
#include "invalid.h"

extern const bld_string bld_command_string_compiler;

typedef enum bld_command_compiler_type {
    BLD_COMMAND_COMPILER_SET_COMPILER,
    BLD_COMMAND_COMPILER_ADD_FLAGS,
    BLD_COMMAND_COMPILER_REMOVE_FLAGS,
    BLD_COMMAND_COMPILER_PRINT,
    BLD_COMMAND_COMPILER_CLEAR
} bld_command_compiler_type;

typedef struct bld_command_compiler {
    bld_string target;
    bld_command_compiler_type type;
    bld_path path;
    bld_string compiler;
    bld_array flags;
} bld_command_compiler;

bld_handle_annotated command_handle_compiler(char*);
int command_compiler_convert(bld_command*, bld_data*, bld_command_compiler*, bld_command_invalid*);
int command_compiler(bld_command_compiler*, bld_data*);
void command_compiler_free(bld_command_compiler*);

#endif
