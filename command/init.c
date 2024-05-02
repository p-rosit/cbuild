#include "../bit_core/os.h"
#include "../bit_core/logging.h"
#include "utils.h"
#include "init.h"

int bld_init_project(bld_path*, bld_args*);
int bld_init_target(bld_path*, bld_args*);

const bld_string bld_command_init = STRING_COMPILE_TIME_PACK("init");

int bld_init(bld_path* root, bld_args* args) {
    log_debug("Initializing!");

    if (args_empty(args)) {
        return bld_init_project(root, args);
    } else if (root->str.size > 0) {
        return bld_init_target(root, args);
    } else {
        log_error("Project has not been initialized");
        return -1;
    }
}

int bld_init_project(bld_path* root, bld_args* args) {
    char cwd[FILENAME_MAX];
    bld_path new_root;
    bld_path build_dir;
    log_debug("Initializing project");
    (void)(args);

    if (root->str.size != 0) {
        log_error("Build has already been initialized!");
        return -1;
    }

    os_cwd(cwd, FILENAME_MAX);
    new_root = path_from_string(cwd);
    build_dir = path_copy(&new_root);
    path_append_string(&build_dir, string_unpack(&bld_build_path));

    os_dir_make(path_to_string(&build_dir));

    path_free(&new_root);
    path_free(&build_dir);
    return -1;
}

int bld_init_target(bld_path* root, bld_args* args) {
    int error;
    bld_path target_path;
    bld_string target_name;
    log_debug("Initializing target");

    if (args_empty(args)) {
        log_error("Missing target name, unreachable error");
        return -1;
    }

    target_name = args_advance(args);
    target_path = path_copy(root);
    path_append_string(&target_path, string_unpack(&bld_build_path));
    path_append_string(&target_path, string_unpack(&bld_target_path));
    os_dir_make(path_to_string(&target_path));

    path_append_string(&target_path, string_unpack(&target_name));
    error = os_dir_make(path_to_string(&target_path));

    if (error) {
        path_free(&target_path);
        log_error("Target already exists");
        return -1;
    }

    path_free(&target_path);
    return 0;
}
