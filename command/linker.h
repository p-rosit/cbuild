#ifndef COMMAND_LINKER_H
#define COMMAND_LINKER_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "handle.h"
#include "invalid.h"

extern const bld_string bld_command_string_linker;

typedef enum bld_command_linker_type {
    BLD_COMMAND_LINKER_SET_LINKER,
    BLD_COMMAND_LINKER_ADD_FLAGS,
    BLD_COMMAND_LINKER_PRINT,
    BLD_COMMAND_LINKER_CLEAR
} bld_command_linker_type;

typedef struct bld_command_linker {
    bld_string target;
    bld_command_linker_type type;
    bld_path path;
    bld_string linker;
    bld_array flags;
} bld_command_linker;

bld_handle_annotated command_handle_linker(char*);
int command_linker_convert(bld_command*, bld_data*, bld_command_linker*, bld_command_invalid*);
int command_linker(bld_command_linker*, bld_data*);
void command_linker_free(bld_command_linker*);

#endif
