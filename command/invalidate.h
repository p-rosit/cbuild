#ifndef COMMAND_INVALIDATE_H
#define COMMAND_INVALIDATE_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_invalidate;

typedef struct bld_command_invalidate {
    bld_string target;
    bld_path path;
} bld_command_invalidate;

int command_invalidate_parse(bld_string*, bld_args*, bld_data*, bld_command_invalidate*, bld_command_invalid*);
int command_invalidate(bld_command_invalidate*, bld_data*);
void command_invalidate_free(bld_command_invalidate*);

#endif
