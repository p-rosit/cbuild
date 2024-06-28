#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "invalidate.h"

const bld_string bld_command_string_invalidate = STRING_COMPILE_TIME_PACK("invalidate");

int command_invalidate(bld_command_invalidate* cmd, bld_data* data) {
    bld_iter iter;
    bld_path* path;
    (void)(data);
    
    log_info("Invalidating target \"%s\"", string_unpack(&cmd->target));
    iter = iter_array(&cmd->paths);
    while (iter_next(&iter, (void**) &path)) {
        log_info("%s", path_to_string(path));
    }

    return 0;
}

int command_invalidate_convert(bld_command* pre_cmd, bld_data* data, bld_command_invalidate* cmd, bld_command_invalid* invalid) {
    int error;
    bld_iter iter;
    bld_string err;
    bld_string* str_path;
    bld_command_positional* arg;
    bld_command_positional_optional* target;
    bld_command_positional_vargs* paths;

    arg = array_get(&pre_cmd->positional, 0);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_invalidate_convert: missing first optional");}
    target = &arg->as.opt;

    if (!utils_get_target(&cmd->target, &err, target, data)) {
        error = -1;
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 2);
    if (arg->type != BLD_HANDLE_POSITIONAL_VARGS) {log_fatal("command_invalidate_convert: no varg");}
    paths = &arg->as.vargs;

    if (paths->values.size <= 0) {
        error = -1;
        err = string_pack("expected paths to add\n");
        err = string_copy(&err);
        goto free_target;
    }

    cmd->paths = array_new(sizeof(bld_path));
    iter = iter_array(&paths->values);
    while (iter_next(&iter, (void**) &str_path)) {
        bld_path path = path_from_string(string_unpack(str_path));
        array_push(&cmd->paths, &path);
    }

    return 0;
    free_target:
    string_free(&cmd->target);
    parse_failed:
    *invalid = command_invalid_new(error, &err);
    return -1;
}

bld_handle_annotated command_handle_invalidate(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_INVALIDATE;
    handle.name = bld_command_string_invalidate;
    handle.handle = handle_new(name);
    handle_positional_optional(&handle.handle, "Target");
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_invalidate));
    handle_positional_vargs(&handle.handle, "Path to invalidate");
    handle_set_description(&handle.handle, "Invalidate paths");

    handle.convert = (bld_command_convert*) command_invalidate_convert;
    handle.execute = (bld_command_execute*) command_invalidate;
    handle.free = (bld_command_free*) command_invalidate_free;

    return handle;
}

void command_invalidate_free(bld_command_invalidate* cmd) {
    bld_iter iter;
    bld_path* path;

    string_free(&cmd->target);

    iter = iter_array(&cmd->paths);
    while (iter_next(&iter, (void**) &path)) {
        path_free(path);
    }
    array_free(&cmd->paths);
}
