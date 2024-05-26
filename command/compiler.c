#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "compiler.h"

const bld_string bld_command_string_compiler = STRING_COMPILE_TIME_PACK("compiler");

int command_compiler(bld_command_compiler* cmd, bld_data* data) {
    bld_iter iter;
    bld_command_flag* flag;
    (void)(data);

    log_info("Configure compiler of \"%s\"", string_unpack(&cmd->target));
    log_info("");
    log_info("Flags:");
    iter = iter_array(&cmd->flags);
    while (iter_next(&iter, (void**) &flag)) {
        log_info("  %s%s", flag->is_switch ? "-" : "--", string_unpack(&flag->flag));
    }

    return 0;
}

int command_compiler_convert(bld_command* pre_cmd, bld_data* data, bld_command_compiler* cmd, bld_command_invalid* invalid) {
    bld_string err, add, remove;
    bld_command_positional* arg;
    bld_command_positional_required* path;
    bld_command_positional_optional* target;
    bld_command_positional_required* option;
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

    arg = array_get(&pre_cmd->positional, 3);
    if (arg->type != BLD_HANDLE_POSITIONAL_REQUIRED) {log_fatal("command_compiler_convert: missing option");}
    option = &arg->as.req;

    add = string_pack("++");
    remove = string_pack("--");
    if (!(string_eq(&option->value, &add) || string_eq(&option->value, &remove))) {
        string_free(&cmd->target);
        err = string_new();
        string_append_string(&err, "Expected option to add or remove with ++/--, got ");
        string_append_string(&err, string_unpack(&option->value));
        string_append_char(&err, '\n');
        goto parse_failed;
    }

    cmd->path = path_from_string(string_unpack(&path->value));
    cmd->flags = pre_cmd->extra_flags;
    cmd->add_flags = string_eq(&option->value, &add);
    pre_cmd->extra_flags = array_new(sizeof(bld_command_flag));

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
    handle_positional_required(&handle.handle, "Add/Remove flags with ++ or --");
    handle_allow_flags(&handle.handle);
    handle_allow_arbitrary_flags(&handle.handle, "Passthrough compiler flags");
    handle_set_description(&handle.handle, "Set compiler flags");

    handle.convert = (bld_command_convert*) command_compiler_convert;
    handle.execute = (bld_command_execute*) command_compiler;
    handle.free = (bld_command_free*) command_compiler_free;

    return handle;
}

void command_compiler_free(bld_command_compiler* cmd) {
    bld_iter iter;
    bld_command_flag* flag;

    string_free(&cmd->target);
    path_free(&cmd->path);

    iter = iter_array(&cmd->flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(&flag->flag);
    }
    array_free(&cmd->flags);
}
