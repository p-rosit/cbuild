#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "../config/config.h"
#include "utils.h"
#include "init.h"

int command_init_project(bld_command_init*, bld_data*);
int command_init_target(bld_command_init*, bld_data*);

const bld_string bld_command_string_init = STRING_COMPILE_TIME_PACK("init");

int command_init(bld_command_init* cmd, bld_data* data) {
    if (cmd->init_project) {
        return command_init_project(cmd, data);
    } else {
        return command_init_target(cmd, data);
    }
}

int command_init_project(bld_command_init* cmd, bld_data* data) {
    char cwd[FILENAME_MAX];
    bld_path new_root;
    bld_path build_dir;
    log_debug("Initializing project");
    (void)(cmd);

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

int command_init_target(bld_command_init* cmd, bld_data* data) {
    int error;
    bld_path target_path;
    bld_path path_main;
    log_debug("Initializing target: \"%s\"", string_unpack(&cmd->target));

    if (!data->has_root) {
        log_fatal("Build has not been initialized!");
        return -1;
    }

    if (set_has(&data->targets, string_hash(string_unpack(&cmd->target)))) {
        log_fatal("Target '%s' already exists", string_unpack(&cmd->target));
    }

    if (!os_file_exists(path_to_string(&cmd->path_main))) {
        log_fatal("Specified main file '%s' is not a path to a file/directory", path_to_string(&cmd->path_main));
    }

    target_path = path_copy(&data->root);
    path_append_string(&target_path, string_unpack(&bld_path_build));
    error = os_dir_make(path_to_string(&target_path));
    if (!error) {
        log_fatal("command_init_target: unreachable error");
    }

    path_append_string(&target_path, string_unpack(&bld_path_target));
    os_dir_make(path_to_string(&target_path));

    path_append_string(&target_path, string_unpack(&cmd->target));
    error = os_dir_make(path_to_string(&target_path));
    if (error) {
        log_fatal("command_init_target: mkdir returned error");
    }

    path_append_string(&target_path, "config.json");
    path_main = path_copy(&cmd->path_main);
    data->target_config_parsed = 1;
    data->target_config = config_target_new(&path_main);
    serialize_config_target(&target_path, &data->target_config);

    path_free(&target_path);
    return 0;
}

int command_init_convert(bld_command* pre_cmd, bld_data* data, bld_command_init* cmd, bld_command_invalid* invalid) {
    bld_string error_msg;
    bld_command_positional* arg;
    bld_command_positional_optional* target;
    (void)(data);

    arg = array_get(&pre_cmd->positional, 1);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_init_convert: missing first optional");}
    target = &arg->as.opt;

    if (!target->present) {
        cmd->init_project = 1;
        return 0;
    } else {
        bld_command_positional_optional* path;

        arg = array_get(&pre_cmd->positional, 2);
        if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_init_convert: missing path optional");}
        path = &arg->as.opt;

        if (!path->present) {
            error_msg = string_pack("missing path to main file\n");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        }

        cmd->init_project = 0;
        cmd->target = string_copy(&target->value);
        cmd->path_main = path_from_string(string_unpack(&path->value));
        return 0;
    }
}

bld_handle_annotated command_handle_init(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_INIT;
    handle.handle = handle_new(name);
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_init));
    handle_positional_optional(&handle.handle, "New target to initialize");
    handle_positional_optional(&handle.handle, "Path to the main file of the target");
    handle_set_description(&handle.handle, "Initialize project or target");

    handle.convert = (bld_command_convert*) command_init_convert;
    handle.execute = (bld_command_execute*) command_init;
    handle.free = (bld_command_free*) command_init_free;

    return handle;
}

void command_init_free(bld_command_init* cmd) {
    if (!cmd->init_project) {
        string_free(&cmd->target);
        path_free(&cmd->path_main);
    }
}
