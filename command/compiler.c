#include "../bld_core/logging.h"
#include "compiler.h"

const bld_string bld_command_string_compiler = STRING_COMPILE_TIME_PACK("compiler");

int command_compiler(bld_command_compiler* cmd, bld_data* data) {
    if (cmd->has_path) {
        log_warn("Configure compilers of target \"%s\" under \"%s\"", string_unpack(&cmd->target), path_to_string(&cmd->path));
    } else {
        log_warn("Configure compilers of target \"%s\"", string_unpack(&cmd->target));
    }
    (void)(data);
    return -1;
}

int command_compiler_parse(bld_string* target, bld_args* args, bld_data* data, bld_command_compiler* cmd, bld_command_invalid* invalid) {
    bld_string path_add, error_msg;
    (void)(data);

    if (args_empty(args)) {
        cmd->target = string_copy(target);
        cmd->has_path = 0;
        return 0;
    }

    path_add = args_advance(args);

    if (!args_empty(args)) {
        error_msg = string_pack("bld: too many inputs");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    cmd->target = string_copy(target);
    cmd->has_path = 1;
    cmd->path = path_from_string(string_unpack(&path_add));
    return 0;
}

void command_compiler_free(bld_command_compiler* cmd) {
    string_free(&cmd->target);
    if (cmd->has_path) {
        path_free(&cmd->path);
    }
}
