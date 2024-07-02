#include "../bld_core/logging.h"
#include "../bld_core/incremental.h"
#include "../bld_core/project_testing.h"
#include "init.h"
#include "build.h"
#include "command_test.h"

const bld_string bld_command_string_test = STRING_COMPILE_TIME_PACK("test");
const bld_string bld_command_test_see_more = STRING_COMPILE_TIME_PACK(
    "See `bld help test` for more information."
);

int command_test(bld_command_test* cmd, bld_data* data) {
    bld_forward_project fproject;
    bld_project project;
    bld_array test_files;

    if (!set_has(&data->targets, string_hash(string_unpack(&cmd->target)))) {
        printf("'%s' is not a known target. %s\n", string_unpack(&cmd->target), string_unpack(&bld_command_test_see_more));
        return -1;
    }

    fproject = command_build_project_new(&cmd->target, data);
    project = project_resolve(&fproject);

    test_files = project_tests_under(&project, &cmd->test_path);
    if (test_files.size == 0) {
        printf("No test files found under '%s'\n", path_to_string(&cmd->test_path));
        project_free(&project);
        return -1;
    }

    project_test_files(&project, &test_files);
    project_save_cache(&project);

    array_free(&test_files);
    project_free(&project);
    return 0;
}

int command_test_convert(bld_command* pre_cmd, bld_data* data, bld_command_test* cmd, bld_command_invalid* invalid) {
    int error;
    bld_string err;
    bld_command_positional* arg;
    bld_command_positional_optional* target;
    bld_command_positional_required* path;

    if (!data->has_root) {
        error = -1;
        err = string_copy(&bld_command_init_missing_project);
        goto parse_failed;
    }

    if (data->targets.size == 0) {
        error = -1;
        err = string_copy(&bld_command_init_no_targets);
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 0);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal(LOG_FATAL_PREFIX "missing no target");}
    target = &arg->as.opt;

    if (!utils_get_target(&cmd->target, &err, target, data)) {
        error = -1;
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 2);
    if (arg->type != BLD_HANDLE_POSITIONAL_REQUIRED) {log_fatal(LOG_FATAL_PREFIX "missing no path");}
    path = &arg->as.req;

    cmd->test_path = path_from_string(string_unpack(&path->value));
    return 0;
    parse_failed:
    *invalid = command_invalid_new(error, &err);
    return -1;
}

bld_handle_annotated command_handle_test(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_TEST;
    handle.name = bld_command_string_test;
    handle.handle = handle_new(name);
    handle_positional_optional(&handle.handle, "The target to modify");
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_test));
    handle_positional_required(&handle.handle, "The path under which all tests will be compiled and executed");
    handle_set_description(
        &handle.handle,
        "Test all test files under root"
    );

    handle.convert = (bld_command_convert*) command_test_convert;
    handle.execute = (bld_command_execute*) command_test;
    handle.free = (bld_command_free*) command_test_free;

    return handle;
}

void command_test_free(bld_command_test* cmd) {
    string_free(&cmd->target);
    path_free(&cmd->test_path);
}
