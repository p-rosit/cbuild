#ifndef COMMAND_IGNORE_H
#define COMMAND_IGNORE_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_ignore;

typedef struct bld_command_ignore {
    bld_string target;
    int remove_flag;
    bld_path path;
} bld_command_ignore;

int command_ignore_parse(bld_string*, bld_args*, bld_data*, bld_command_ignore*, bld_command_invalid*);
int command_ignore(bld_command_ignore*, bld_data*);
void command_ignore_free(bld_command_ignore*);

#endif
