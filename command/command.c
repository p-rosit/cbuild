#include "../bld_core/logging.h"
#include "../config/config_target.h"
#include "command.h"
#include "handle.h"

typedef int (bld_command_convert)(bld_command*, bld_data*, void*, bld_command_invalid*);
typedef int (bld_command_execute)(void*, bld_data*);
typedef void (bld_command_free)(void*);

typedef struct bld_handle_named {
    bld_command_type type;
    bld_handle handle;
    bld_command_convert* convert;
    bld_command_execute* execute;
    bld_command_free* free;
} bld_handle_named;

bld_set application_available_set(char* name) {
    bld_set cmds = set_new(sizeof(bld_handle_named));
    bld_handle_named handle;

    handle.type = BLD_COMMAND_ADD;
    handle.handle = command_handle_add(name);
    handle.convert = (bld_command_convert*) command_add_convert;
    handle.execute = (bld_command_execute*) command_add;
    handle.free = (bld_command_free*) command_add_free;
    set_add(&cmds, BLD_COMMAND_ADD, &handle);

    handle.type = BLD_COMMAND_INVALID;
    handle.execute = (bld_command_execute*) command_invalid;
    handle.free = (bld_command_free*) command_invalid_free;
    set_add(&cmds, BLD_COMMAND_INVALID, &handle);

    return cmds;
}

