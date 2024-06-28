#include "../bld_core/logging.h"
#include "init.h"
#include "switch.h"

const bld_string bld_command_string_switch = STRING_COMPILE_TIME_PACK("switch");

int command_switch(bld_command_switch* cmd, bld_data* data) {
    if (!data->config_parsed) {
        log_fatal("command_switch: config not parsed");
    }

    if (data->config.active_target_configured) {
        if (string_eq(&cmd->target, &data->config.active_target)) {
            log_info("Target %s already active", string_unpack(&cmd->target));
            return 0;
        }
    }

    if (!set_has(&data->targets, string_hash(string_unpack(&cmd->target)))) {
        log_error("%s is not a known target", string_unpack(&cmd->target));
        return -1;
    }

    if (data->config.active_target_configured) {
        string_free(&data->config.active_target);
    }

    data->config.active_target_configured = 1;
    data->config.active_target = string_copy(&cmd->target);
    config_save(data);
    printf("Switched to '%s'\n", string_unpack(&cmd->target));

    return 0;
}

int command_switch_convert(bld_command* pre_cmd, bld_data* data, bld_command_switch* cmd, bld_command_invalid* invalid) {
    int error;
    bld_string err;
    bld_command_positional* arg;
    bld_command_positional_required* target;

    if (!data->has_root) {
        error = -1;
        err = string_copy(&bld_command_init_missing_project);
        goto parse_failed;
    }

    if (data->targets.size == 0) {
        error = -1;
        err = string_copy(&bld_command_init_no_targets);
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 1);
    if (arg->type != BLD_HANDLE_POSITIONAL_REQUIRED) {log_fatal("command_switch_convert: missing first convert");}
    target = &arg->as.req;

    cmd->target = string_copy(&target->value);
    return 0;
    parse_failed:
    *invalid = command_invalid_new(error, &err);
    return -1;
}

bld_handle_annotated command_handle_switch(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_SWITCH;
    handle.name = bld_command_string_switch;
    handle.handle = handle_new(name);
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_switch));
    handle_positional_required(&handle.handle, "The target to switch to");
    handle_set_description(&handle.handle, "Switch to a new active target");

    handle.convert = (bld_command_convert*) command_switch_convert;
    handle.execute = (bld_command_execute*) command_switch;
    handle.free = (bld_command_free*) command_switch_free;

    return handle;
}

void command_switch_free(bld_command_switch* cmd) {
    string_free(&cmd->target);
}
