#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "status.h"

const bld_string bld_command_string_status = STRING_COMPILE_TIME_PACK("status");

int command_status(bld_command_status* status, bld_data* data) {
    bld_iter iter = iter_set(&data->targets);
    bld_string* target;

    if (status->target_status) {
        log_info("Status: %s", string_unpack(&status->target));
    } else {
            log_info("Targets in project:");
            while (iter_next(&iter, (void**) &target)) {
            log_info("    %s", string_unpack(target));
        }
    }

    return 0;
}

int command_status_parse(bld_args* args, bld_data* data, bld_command_status* status, bld_command_invalid* invalid) {
    bld_string error_msg;
    bld_string target;

    if (args_empty(args)) {
        status->target_status = 0;
        return 0;
    }

    target = args_advance(args);
    if (!set_has(&data->targets, string_hash(string_unpack(&target)))) {
        error_msg = string_new();
        string_append_string(&error_msg, "bld: optional second argument of status, '");
        string_append_string(&error_msg, string_unpack(&target));
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
    status->target = string_copy(&target);
    return 0;
}

void command_status_free(bld_command_status* status) {
    if (status->target_status) {
        string_free(&status->target);
    }
}
