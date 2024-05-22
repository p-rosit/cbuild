#ifndef COMMAND_REMOVE_H
#define COMMAND_REMOVE_H
#include "../bld_core/args.h"
#include "../bld_core/path.h"
#include "invalid.h"

extern const bld_string bld_command_string_remove;

typedef struct bld_command_remove {
    bld_string target;
} bld_command_remove;

bld_handle_annotated command_handle_remove(char*);
int command_remove_convert(bld_command*, bld_data*, bld_command_remove*, bld_command_invalid*);
int command_remove(bld_command_remove*, bld_data*);
void command_remove_free(bld_command_remove*);

#endif
