#include "../bit_core/logging.h"
#include "command.h"

bld_command command_parse(bld_args* args) {
    int error;
    bld_string base_command;
    bld_string error_msg;
    bld_command cmd;

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
        error = command_init_parse(args, &cmd.as.init, &cmd.as.invalid);
    } else if (string_eq(&base_command, &bld_command_string_remove)) {
        cmd.type = BLD_COMMAND_REMOVE;
        error = command_remove_parse(args, &cmd.as.remove, &cmd.as.invalid);
    } else {
        error = 1;
        error_msg = string_pack("bit: '");
        error_msg = string_copy(&error_msg);
        string_append_string(&error_msg, string_unpack(&base_command));
        string_append_string(&error_msg, "' is not a bit command");
        cmd.as.invalid = command_invalid_new(-1, &error_msg);
    }

    if (error) {
        cmd.type = BLD_COMMAND_INVALID;
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
        default:
            log_fatal("command_free: Unreachable error");
    }
}
