#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "status.h"

const bld_string bld_command_string_status = STRING_COMPILE_TIME_PACK("status");

int command_status_all(bld_command_status*, bld_data*);
int command_status_target(bld_command_status*, bld_data*);

int command_status(bld_command_status* status, bld_data* data) {
    if (status->target_status) {
        return command_status_target(status, data);
    } else {
        return command_status_all(status, data);
    }
}

int command_status_all(bld_command_status* status, bld_data* data) {
    bld_iter iter;
    bld_string* target;
    (void)(status);
    log_info("Targets in project:");

    iter = iter_set(&data->targets);
    while (iter_next(&iter, (void**) &target)) {
        int is_default = 0;
        if (data->config.default_target_configured) {
            if (string_eq(target, &data->config.target)) {
                is_default = 1;
            }
        }

        if (is_default) {
            log_info("  * %s", string_unpack(target));
        } else {
            log_info("    %s", string_unpack(target));
        }
    }
    return 0;
}

int command_status_target(bld_command_status* status, bld_data* data) {
    bld_config_target* config = &data->target_config;
    bld_iter iter;
    bld_path* path;
    log_info("Target: %s", string_unpack(&status->target));

    return 0;
}

int command_status_parse(bld_args* args, bld_data* data, bld_command_status* status, bld_command_invalid* invalid) {
    bld_string error_msg, target_cmd, *target;
    (void)(data);

    if (args_empty(args)) {
        status->target_status = 0;
        return 0;
    } else {
        target_cmd = args_advance(args);
        target = &target_cmd;

        if (data->target_config_parsed) {
            log_fatal("command_status_parse: target config already parsed, unreachable error");
        }

        data->target_config_parsed = !config_target_load(data, target, &data->target_config);

        if (!data->target_config_parsed) {
            error_msg = string_new();
            string_append_string(&error_msg, "bld: could not parse config of target '");
            string_append_string(&error_msg, string_unpack(target));
            string_append_string(&error_msg, "'");
            invalid->code = -1;
            invalid->msg = error_msg;
            return -1;
        }
    }

    if (!set_has(&data->targets, string_hash(string_unpack(target)))) {
        error_msg = string_new();
        string_append_string(&error_msg, "bld: optional second argument of status, '");
        string_append_string(&error_msg, string_unpack(target));
        string_append_string(&error_msg, "' is not a target");
        invalid->code = -1;
        invalid->msg = error_msg;
        return -1;
    }

    if (!args_empty(args)) {
        bld_string error_msg = string_new();
        string_append_string(&error_msg, "bld: status takes no arguments other than an optional target");
        invalid->code = -1;
        invalid->msg = error_msg;
        return -1;
    }

    status->target_status = 1;
    status->target = string_copy(target);
    return 0;
}

void command_status_free(bld_command_status* status) {
    if (status->target_status) {
        string_free(&status->target);
    }
}
