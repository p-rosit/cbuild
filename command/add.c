#include "../bld_core/logging.h"
#include "add.h"

const bld_string bld_command_string_add = STRING_COMPILE_TIME_PACK("add");

int command_add(bld_command_add* add, bld_data* data) {
    log_warn("Adding: \"%s\" to target \"%s\"", path_to_string(&add->path), string_unpack(&add->target));
    (void)(data);
    return -1;
}

int command_add_parse(bld_string* target, bld_args* args, bld_data* data, bld_command_add* add, bld_command_invalid* invalid) {
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

    add->target = string_copy(target);
    add->path = path_from_string(string_unpack(&path_add));
    return 0;
}

void command_add_free(bld_command_add* add) {
    string_free(&add->target);
    path_free(&add->path);
}
