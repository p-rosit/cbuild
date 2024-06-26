#ifndef COMMAND_STATUS_H
#define COMMAND_STATUS_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "invalid.h"

extern const bld_string bld_command_string_status;

typedef struct bld_command_status {
    int target_status;
    bld_string target;
} bld_command_status;

bld_handle_annotated command_handle_status(char*);
int command_status_convert(bld_command*, bld_data*, bld_command_status*, bld_command_invalid*);
int command_status(bld_command_status*, bld_data*);
void command_status_free(bld_command_status*);

#endif
