#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "utils.h"
#include "init.h"

int command_init_project(bld_command_init*, bld_data*);
int command_init_target(bld_command_init*, bld_data*);

const bld_string bld_command_string_init = STRING_COMPILE_TIME_PACK("init");

int command_init(bld_command_init* init, bld_data* data) {
    if (init->init_project) {
        return command_init_project(init, data);
    } else {
        return command_init_target(init, data);
    }
}

int command_init_project(bld_command_init* init, bld_data* data) {
    char cwd[FILENAME_MAX];
    bld_path new_root;
    bld_path build_dir;
    log_debug("Initializing project");
    (void)(init);
    (void)(data);

    if (data->root.str.size != 0) {
        log_error("Build has already been initialized!");
        return -1;
    }

    os_cwd(cwd, FILENAME_MAX);
    new_root = path_from_string(cwd);
    build_dir = path_copy(&new_root);
    path_append_string(&build_dir, string_unpack(&bld_path_build));

    os_dir_make(path_to_string(&build_dir));

    path_free(&new_root);
    path_free(&build_dir);
    return 0;
}

int command_init_target(bld_command_init* init, bld_data* data) {
    int error;
    bld_path target_path;
    log_debug("Initializing target: \"%s\"", string_unpack(&init->target));

    target_path = path_copy(&data->root);
    path_append_string(&target_path, string_unpack(&bld_path_build));
    error = os_dir_make(path_to_string(&target_path));
    if (!error) {
        log_fatal("command_init_target: unreachable error");
    }

    path_append_string(&target_path, string_unpack(&bld_path_target));
    os_dir_make(path_to_string(&target_path));

    path_append_string(&target_path, string_unpack(&init->target));
    error = os_dir_make(path_to_string(&target_path));

    path_free(&target_path);

    if (error) {
        log_error("Target already exists");
        return -1;
    }
    return 0;
}

int command_init_parse(bld_args* args, bld_data* data, bld_command_init* init, bld_command_invalid* invalid) {
    bld_string target;
    bld_string error_msg;

    if (args_empty(args)) {
        init->init_project = 1;
        return 0;
    } else {
        target = args_advance(args);

        if (!args_empty(args)) {
            error_msg = string_pack("invalid nargs");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        }

        init->init_project = 0;
        init->target = string_copy(&target);
        return 0;
    }
}

void command_init_free(bld_command_init* init) {
    if (!init->init_project) {
        string_free(&init->target);
    }
}
