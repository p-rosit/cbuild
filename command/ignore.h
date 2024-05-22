#ifndef COMMAND_IGNORE_H
#define COMMAND_IGNORE_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "handle.h"
#include "invalid.h"

extern const bld_string bld_command_string_ignore;

typedef struct bld_command_ignore {
    bld_string target;
    int remove_flag;
    bld_array paths;
} bld_command_ignore;

bld_handle_annotated command_handle_ignore(char*);
int command_ignore_convert(bld_command*, bld_data*, bld_command_ignore*, bld_command_invalid*);
int command_ignore(bld_command_ignore*, bld_data*);
void command_ignore_free(bld_command_ignore*);

#endif
