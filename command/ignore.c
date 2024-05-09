#include "../bld_core/logging.h"
#include "ignore.h"

const bld_string bld_command_string_ignore = STRING_COMPILE_TIME_PACK("ignore");

int command_ignore(bld_command_ignore* cmd, bld_data* data) {
    log_warn("Ignoring: \"%s\" for target \"%s\"", path_to_string(&cmd->path), string_unpack(&cmd->target));
    (void)(data);
    return -1;
}

int command_ignore_parse(bld_string* target, bld_args* args, bld_data* data, bld_command_ignore* cmd, bld_command_invalid* invalid) {
    bld_string path_add, error_msg;
    (void)(data);

    if (args_empty(args)) {
        error_msg = string_pack("bld: missing path");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    path_add = args_advance(args);

    if (!args_empty(args)) {
        error_msg = string_pack("bld: too many inputs");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    cmd->target = string_copy(target);
    cmd->path = path_from_string(string_unpack(&path_add));
    return 0;
}

void command_ignore_free(bld_command_ignore* cmd) {
    string_free(&cmd->target);
    path_free(&cmd->path);
}
