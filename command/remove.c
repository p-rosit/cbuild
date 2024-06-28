#include "../bld_core/logging.h"
#include "invalid.h"
#include "init.h"
#include "remove.h"

const bld_string bld_command_string_remove = STRING_COMPILE_TIME_PACK("remove");

int command_remove(bld_command_remove* remove, bld_data* data) {
    if (data->config_parsed) {
        if (!data->config.active_target_configured) {
            if (string_eq(&remove->target, &data->config.active_target)) {
                log_error("Cannot remove currently active target");
                return -1;
            }
        }
    }

    log_debug("Removing target: \"%s\"", string_unpack(&remove->target));
    return 0;
}

int command_remove_convert(bld_command* pre_cmd, bld_data* data, bld_command_remove* cmd, bld_command_invalid* invalid) {
    bld_string err;
    bld_command_positional* arg;
    bld_command_positional_required* target;

    if (!data->has_root) {
        err = string_copy(&bld_command_init_missing_project);
        goto parse_failed;
    }

    if (data->targets.size == 0) {
        err = string_copy(&bld_command_init_no_targets);
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 1);
    if (arg->type != BLD_HANDLE_POSITIONAL_REQUIRED) {log_fatal("command_switch_convert: missing first convert");}
    target = &arg->as.req;

    cmd->target = string_copy(&target->value);
    return 0;
    parse_failed:
    *invalid = command_invalid_new(-1, &err);
    return -1;
}

bld_handle_annotated command_handle_remove(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_REMOVE;
    handle.name = bld_command_string_remove;
    handle.handle = handle_new(name);
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_remove));
    handle_positional_required(&handle.handle, "The target to remove");
    handle_set_description(&handle.handle, "Remove a target");

    handle.convert = (bld_command_convert*) command_remove_convert;
    handle.execute = (bld_command_execute*) command_remove;
    handle.free = (bld_command_free*) command_remove_free;

    return handle;
}

void command_remove_free(bld_command_remove* remove) {
    string_free(&remove->target);
}
