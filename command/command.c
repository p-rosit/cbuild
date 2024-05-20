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

int command_implicit_target_parse(bld_string*, bld_args*, bld_data*, bld_command*, bld_command_invalid*);

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

bld_command command_parse(bld_args* args, bld_data* data) {
    int error;
    bld_string base_command;
    bld_string error_msg;
    bld_command cmd;
    bld_command_invalid invalid;

    if (args_empty(args) && data->config_parsed && data->config.default_target_configured) {
        data->target_config_parsed = !config_target_load(data, &data->config.target, &data->target_config);

        if (!data->target_config_parsed) {
            error_msg = string_new();
            string_append_string(&error_msg, "bld: could not parse config file of active target '");
            string_append_string(&error_msg, string_unpack(&data->config.target));
            string_append_string(&error_msg, "'");
            error = -1;
            invalid.code = -1;
            invalid.msg = error_msg;
        } else {
            cmd.type = BLD_COMMAND_BUILD;
            error = command_build_parse(&data->config.target, args, data, &cmd.as.build, &invalid);
        }

        if (error) {
            cmd.type = BLD_COMMAND_INVALID;
            cmd.as.invalid = invalid;
        }

        return cmd;
    } else if (args_empty(args)) {
        error_msg = string_pack("no default target configured to build");
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
        string_append_string(&error_msg, "bld: not in a bld project");
        invalid.code = -1;
        invalid.msg = error_msg;
    } else if (string_eq(&base_command, &bld_command_string_switch)) {
        cmd.type = BLD_COMMAND_SWITCH;
        error = command_switch_parse(args, data, &cmd.as.cmd_switch, &invalid);
    } else if (string_eq(&base_command, &bld_command_string_remove)) {
        cmd.type = BLD_COMMAND_REMOVE;
        error = command_remove_parse(args, data, &cmd.as.remove, &invalid);
    } else if (string_eq(&base_command, &bld_command_string_help)) {
        cmd.type = BLD_COMMAND_HELP;
        error = command_help_parse(args, data, &cmd.as.help, &invalid);
    } else if (string_eq(&base_command, &bld_command_string_status)) {
        cmd.type = BLD_COMMAND_STATUS;
        error = command_status_parse(args, data, &cmd.as.status, &invalid);
    } else {
        error = command_implicit_target_parse(&base_command, args, data, &cmd, &invalid);
    }

    if (error) {
        cmd.type = BLD_COMMAND_INVALID;
        cmd.as.invalid = invalid;
    }

    return cmd;
}

int command_implicit_target_parse(bld_string* base_command, bld_args* args, bld_data* data, bld_command* cmd, bld_command_invalid* invalid) {
    int build_command = 0;
    bld_string error_msg;
    bld_string* target;
    bld_string next_command;

    if (set_has(&data->targets, string_hash(string_unpack(base_command)))) {
        target = base_command;
        if (!args_empty(args)) {
            next_command = args_advance(args);
        } else {
            build_command = 1;
        }
    } else {
        if (!data->config.default_target_configured) {
            error_msg = string_pack("bld: no default target");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        }

        target = &data->config.target;
        next_command = *base_command;
    }

    {
        data->target_config_parsed = !config_target_load(data, target, &data->target_config);

        if (!data->target_config_parsed) {
            error_msg = string_new();
            string_append_string(&error_msg, "bld: could not parse config file of target '");
            string_append_string(&error_msg, string_unpack(target));
            string_append_string(&error_msg, "'");
            invalid->code = -1;
            invalid->msg = error_msg;
            return -1;
        }
    }

    if (build_command) {
        cmd->type = BLD_COMMAND_BUILD;
        return command_build_parse(target, args, data, &cmd->as.build, invalid);
    } else if (
        string_eq(&next_command, &bld_command_string_init)
        || string_eq(&next_command, &bld_command_string_switch)
        || string_eq(&next_command, &bld_command_string_remove)
        || string_eq(&next_command, &bld_command_string_help)
        || string_eq(&next_command, &bld_command_string_status)
    ) {
        error_msg = string_new();
        string_append_string(&error_msg, "bld: command '");
        string_append_string(&error_msg, string_unpack(&next_command));
        string_append_string(&error_msg, "' does not take initial target argument");
        invalid->code = -1;
        invalid->msg = error_msg;
        return -1;
    } else if (string_eq(&next_command, &bld_command_string_add)) {
        cmd->type = BLD_COMMAND_ADD;
        return command_add_parse(target, args, data, &cmd->as.add, invalid);
    } else if (string_eq(&next_command, &bld_command_string_compiler)) {
        cmd->type = BLD_COMMAND_COMPILER;
        return command_compiler_parse(target, args, data, &cmd->as.compiler, invalid);
    } else if (string_eq(&next_command, &bld_command_string_ignore)) {
        cmd->type = BLD_COMMAND_IGNORE;
        return command_ignore_parse(target, args, data, &cmd->as.ignore, invalid);
    } else if (string_eq(&next_command, &bld_command_string_invalidate)) {
        cmd->type = BLD_COMMAND_INVALIDATE;
        return command_invalidate_parse(target, args, data, &cmd->as.invalidate, invalid);
    } else if (string_eq(&next_command, &bld_command_string_linker)) {
        cmd->type = BLD_COMMAND_LINKER;
        return command_linker_parse(target, args, data, &cmd->as.linker, invalid);
    } else {
        error_msg = string_pack("bld: '");
        error_msg = string_copy(&error_msg);
        string_append_string(&error_msg, string_unpack(base_command));
        string_append_string(&error_msg, "' is not a bld command or target");
        *invalid = command_invalid_new(-1, &error_msg);
        return -1;
    }

    log_fatal("command_implicit_target_parse: unreachable error");
    return -1;
}

int command_execute(bld_command* cmd, bld_data* data) {
    switch (cmd->type) {
        case (BLD_COMMAND_INVALID):
            return command_invalid(&cmd->as.invalid, data);
        case (BLD_COMMAND_INIT):
            return command_init(&cmd->as.init, data);
        case (BLD_COMMAND_SWITCH):
            return command_switch(&cmd->as.cmd_switch, data);
        case (BLD_COMMAND_REMOVE):
            return command_remove(&cmd->as.remove, data);
        case (BLD_COMMAND_HELP):
            return command_help(&cmd->as.help, data);
        case (BLD_COMMAND_ADD):
            return command_add(&cmd->as.add, data);
        case (BLD_COMMAND_BUILD):
            return command_build(&cmd->as.build, data);
        case (BLD_COMMAND_COMPILER):
            return command_compiler(&cmd->as.compiler, data);
        case (BLD_COMMAND_IGNORE):
            return command_ignore(&cmd->as.ignore, data);
        case (BLD_COMMAND_INVALIDATE):
            return command_invalidate(&cmd->as.invalidate, data);
        case (BLD_COMMAND_LINKER):
            return command_linker(&cmd->as.linker, data);
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
        case (BLD_COMMAND_SWITCH):
            command_switch_free(&cmd->as.cmd_switch);
            return;
        case (BLD_COMMAND_REMOVE):
            command_remove_free(&cmd->as.remove);
            return;
        case (BLD_COMMAND_HELP):
            command_help_free(&cmd->as.help);
            return;
        case (BLD_COMMAND_ADD):
            command_add_free(&cmd->as.add);
            return;
        case (BLD_COMMAND_BUILD):
            command_build_free(&cmd->as.build);
            return;
        case (BLD_COMMAND_COMPILER):
            command_compiler_free(&cmd->as.compiler);
            return;
        case (BLD_COMMAND_IGNORE):
            command_ignore_free(&cmd->as.ignore);
            return;
        case (BLD_COMMAND_INVALIDATE):
            command_invalidate_free(&cmd->as.invalidate);
            return;
        case (BLD_COMMAND_LINKER):
            command_linker_free(&cmd->as.linker);
            return;
        case (BLD_COMMAND_STATUS):
            command_status_free(&cmd->as.status);
            return;
        default:
            log_fatal("command_free: Unreachable error");
    }
}
