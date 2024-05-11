#include "../bld_core/logging.h"
#include "switch.h"

const bld_string bld_command_string_switch = STRING_COMPILE_TIME_PACK("switch");

int command_switch(bld_command_switch* cmd, bld_data* data) {
    if (!data->config_parsed) {
        log_fatal("command_switch: config not parsed");
    }

    if (data->config.default_target_configured) {
        if (string_eq(&cmd->target, &data->config.target)) {
            log_info("Target %s already active", string_unpack(&cmd->target));
            return 0;
        }
    }

    if (!set_has(&data->targets, string_hash(string_unpack(&cmd->target)))) {
        log_error("%s is not a known target", string_unpack(&cmd->target));
        return -1;
    }

    log_info("Switched to %s", string_unpack(&cmd->target));
    string_free(&data->config.target);
    data->config.target = string_copy(&cmd->target);
    config_save(data, &data->config);

    return 0;
}

int command_switch_parse(bld_args* args, bld_data* data, bld_command_switch* cmd, bld_command_invalid* invalid) {
    bld_string target, error_msg;
    (void)(data);

    if (args_empty(args)) {
        error_msg = string_pack("bld: missing target");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    target = args_advance(args);

    if (!args_empty(args)) {
        error_msg = string_pack("bld: too many inputs");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    cmd->target = string_copy(&target);
    return 0;
}

void command_switch_free(bld_command_switch* cmd) {
    string_free(&cmd->target);
}
