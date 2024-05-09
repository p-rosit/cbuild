#include "../bld_core/logging.h"
#include "switch.h"

const bld_string bld_command_string_switch = STRING_COMPILE_TIME_PACK("switch");

int command_switch(bld_command_switch* cmd, bld_data* data) {
    log_warn("Switching to \"%s\"", string_unpack(&cmd->target));
    (void)(data);
    return -1;
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
