#include "../bld_core/logging.h"
#include "compiler.h"

const bld_string bld_command_string_compiler = STRING_COMPILE_TIME_PACK("compiler");

int command_compiler(bld_command_compiler* cmd, bld_data* data) {
    (void)(data);
    log_info("Configure compiler of \"%s\"", string_unpack(&cmd->target));
    return -1;
}

int command_compiler_convert(bld_command* pre_cmd, bld_data* data, bld_command_compiler* cmd, bld_command_invalid* invalid) {
    bld_string err;
    bld_command_positional* arg;
    bld_command_positional_required* path;
    bld_command_positional_optional* target;
    (void)(data);

    arg = array_get(&pre_cmd->positional, 0);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_compiler_convert: missing first optional");}
    target = &arg->as.opt;

    if (!utils_get_target(&cmd->target, &err, target, data)) {
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 2);
    if (arg->type != BLD_HANDLE_POSITIONAL_REQUIRED) {log_fatal("command_compiler_convert: missing required");}
    path = &arg->as.req;

    cmd->path = path_from_string(string_unpack(&path->value));

    return 0;
    parse_failed:
    *invalid = command_invalid_new(-1, &err);
    return -1;
}

bld_handle_annotated command_handle_compiler(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_COMPILER;
    handle.handle = handle_new(name);
    handle_positional_optional(&handle.handle, "Target to set compiler flags for");
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_compiler));
    handle_positional_required(&handle.handle, "Directory/File to set flags for");
    handle_allow_arbitrary_flags(&handle.handle, "Passthrough compiler flags");
    handle_set_description(&handle.handle, "Set compiler flags");

    handle.convert = (bld_command_convert*) command_compiler_convert;
    handle.execute = (bld_command_execute*) command_compiler;
    handle.free = (bld_command_free*) command_compiler_free;

    return handle;
}

void command_compiler_free(bld_command_compiler* cmd) {
    string_free(&cmd->target);
    path_free(&cmd->path);
}
