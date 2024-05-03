#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "status.h"

const bld_string bld_command_string_status = STRING_COMPILE_TIME_PACK("status");

int command_status(bld_command_status* status, bld_data* data) {
    bld_iter iter = iter_set(&data->targets);
    bld_string* target;
    (void)(status);

    log_info("Targets in project:");
    while (iter_next(&iter, (void**) &target)) {
        log_info("    %s", string_unpack(target));
    }

    return 0;
}

int command_status_parse(bld_args* args, bld_data* data, bld_command_status* status, bld_command_invalid* invalid) {
    if (!args_empty(args)) {
        bld_string error_msg = string_new();
        string_append_string(&error_msg, "bld: status takes no arguments");
        invalid->code = -1;
        invalid->msg = error_msg;
        return -1;
    }

    (void)(data);
    (void)(status);

    return 0;
}

void command_status_free(bld_command_status* status) {
    (void)(status);
}
