#ifndef COMMAND_INVALID_H
#define COMMAND_INVALID_H
#include "../bld_core/dstr.h"
#include "utils.h"

typedef struct bld_command_invalid {
    int code;
    bld_string msg;
} bld_command_invalid;

bld_command_invalid command_invalid_new(int, bld_string*);
int                 command_invalid(bld_command_invalid*, bld_data*);
void                command_invalid_free(bld_command_invalid*);

#endif
