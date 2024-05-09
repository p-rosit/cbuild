#ifndef COMMAND_LINKER_H
#define COMMAND_LINKER_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_linker;

typedef struct bld_command_linker {
    bld_string target;
    int has_path;
    bld_path path;
} bld_command_linker;

int command_linker_parse(bld_string*, bld_args*, bld_data*, bld_command_linker*, bld_command_invalid*);
int command_linker(bld_command_linker*, bld_data*);
void command_linker_free(bld_command_linker*);

#endif
