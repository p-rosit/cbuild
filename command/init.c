#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "../config/config.h"
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

    if (data->has_root) {
        log_fatal("Build has already been initialized!");
        return -1;
    }
    if (data->config_parsed) {
        log_fatal("Unreachable error");
        return -1;
    }

    os_cwd(cwd, FILENAME_MAX);
    new_root = path_from_string(cwd);
    build_dir = path_copy(&new_root);
    path_append_string(&build_dir, string_unpack(&bld_path_build));

    os_dir_make(path_to_string(&build_dir));

    path_append_string(&build_dir, "config.json");
    data->config_parsed = 1;
    data->config = config_new();
    serialize_config(&build_dir, &data->config);

    path_free(&new_root);
    path_free(&build_dir);
    return 0;
}

int command_init_target(bld_command_init* init, bld_data* data) {
    int error;
    bld_path target_path;
    bld_path path_main;
    log_debug("Initializing target: \"%s\"", string_unpack(&init->target));

    if (!data->has_root) {
        log_fatal("Build has not been initialized!");
        return -1;
    }
    if (data->target_config_parsed) {
        log_fatal("Target already exists");
        return -1;
    }

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
    if (error) {
        log_fatal("command_init_target: mkdir returned error");
    }

    path_append_string(&target_path, "config.json");
    path_main = path_copy(&init->path_main);
    data->target_config_parsed = 1;
    data->target_config = config_target_new(&path_main);
    serialize_config_target(&target_path, &data->target_config);

    path_free(&target_path);
    return 0;
}

int command_init_parse(bld_args* args, bld_data* data, bld_command_init* init, bld_command_invalid* invalid) {
    bld_string error_msg;

    if (args_empty(args)) {
        if (data->has_root) {
            error_msg = string_new();
            string_append_string(&error_msg, "bld: project has already been initialized");
            invalid->code = -1;
            invalid->msg = error_msg;
            return -1;
        }

        init->init_project = 1;
        return 0;
    } else {
        bld_string target;
        bld_string path_main;

        if (!data->has_root) {
            error_msg = string_new();
            string_append_string(&error_msg, "bld: project has not been initialized");
            invalid->code = -1;
            invalid->msg = error_msg;
            return -1;
        }

        target = args_advance(args);

        if (args_empty(args)) {
            error_msg = string_pack("missing path");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        }

        path_main = args_advance(args);

        if (!args_empty(args)) {
            error_msg = string_pack("invalid nargs");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        }

        if (set_has(&data->targets, string_hash(string_unpack(&target)))) {
            error_msg = string_new();
            string_append_string(&error_msg, "target '");
            string_append_string(&error_msg, string_unpack(&target));
            string_append_string(&error_msg, "' already exists");
            invalid->code = -1;
            invalid->msg = error_msg;
            return -1;
        }

        if (!os_file_exists(string_unpack(&path_main))) {
            error_msg = string_new();
            string_append_string(&error_msg, "specified main file '");
            string_append_string(&error_msg, string_unpack(&path_main));
            string_append_string(&error_msg, "' is not a path or file");
            invalid->code = -1;
            invalid->msg = error_msg;
            return -1;
        }

        init->init_project = 0;
        init->target = string_copy(&target);
        init->path_main = path_from_string(string_unpack(&path_main));
        return 0;
    }
}

void command_init_free(bld_command_init* init) {
    if (!init->init_project) {
        string_free(&init->target);
        path_free(&init->path_main);
    }
}
