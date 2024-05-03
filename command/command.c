#include "../bld_core/logging.h"
#include "command.h"

bld_command command_parse(bld_args* args, bld_data* data) {
    int error;
    bld_string base_command;
    bld_string error_msg;
    bld_command cmd;
    bld_command_invalid invalid;

    if (args_empty(args)) {
        error_msg = string_pack("missing sub command");
        error_msg = string_copy(&error_msg);
        cmd.type = BLD_COMMAND_INVALID;
        cmd.as.invalid = command_invalid_new(-1, &error_msg);
        return cmd;
    }

    base_command = args_advance(args);
    if (string_eq(&base_command, &bld_command_string_init)) {
        cmd.type = BLD_COMMAND_INIT;
        error = command_init_parse(args, data, &cmd.as.init, &invalid);
    } else if (!data->has_root) {
        error = 1;
        error_msg = string_new();
        string_append_string(&error_msg, "bld: not a bld project");
        invalid.code = -1;
        invalid.msg = error_msg;
    } else if (string_eq(&base_command, &bld_command_string_remove)) {
        cmd.type = BLD_COMMAND_REMOVE;
        error = command_remove_parse(args, data, &cmd.as.remove, &invalid);
    } else if (string_eq(&base_command, &bld_command_string_status)) {
        cmd.type = BLD_COMMAND_STATUS;
        error = command_status_parse(args, data, &cmd.as.status, &invalid);
    } else if (set_has(&data->targets, string_hash(string_unpack(&base_command)))) {
        cmd.type = BLD_COMMAND_BUILD;
        error = command_build_parse(&base_command, args, data, &cmd.as.build, &invalid);
    } else {
        error = 1;
        error_msg = string_pack("bld: '");
        error_msg = string_copy(&error_msg);
        string_append_string(&error_msg, string_unpack(&base_command));
        string_append_string(&error_msg, "' is not a bld command or target");
        invalid = command_invalid_new(-1, &error_msg);
    }

    if (error) {
        cmd.type = BLD_COMMAND_INVALID;
        cmd.as.invalid = invalid;
    }

    return cmd;
}

int command_execute(bld_command* cmd, bld_data* data) {
    switch (cmd->type) {
        case (BLD_COMMAND_INVALID):
            return command_invalid(&cmd->as.invalid, data);
        case (BLD_COMMAND_INIT):
            return command_init(&cmd->as.init, data);
        case (BLD_COMMAND_REMOVE):
            return command_remove(&cmd->as.remove, data);
        case (BLD_COMMAND_BUILD):
            return command_build(&cmd->as.build, data);
        case (BLD_COMMAND_STATUS):
            return command_status(&cmd->as.status, data);
        default:
            log_fatal("command_execute: Unreachable error");
            return -1;
    }
}

void command_free(bld_command* cmd) {
    switch (cmd->type) {
        case (BLD_COMMAND_INVALID):
            command_invalid_free(&cmd->as.invalid);
            return;
        case (BLD_COMMAND_INIT):
            command_init_free(&cmd->as.init);
            return;
        case (BLD_COMMAND_REMOVE):
            command_remove_free(&cmd->as.remove);
            return;
        case (BLD_COMMAND_BUILD):
            command_build_free(&cmd->as.build);
            return;
        case (BLD_COMMAND_STATUS):
            command_status_free(&cmd->as.status);
            return;
        default:
            log_fatal("command_free: Unreachable error");
    }
}
