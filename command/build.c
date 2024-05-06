#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "../bld_core/project.h"
#include "../bld_core/incremental.h"
#include "build.h"

int command_build_target(bld_command_build*, bld_data*);
int command_build_compiler(bld_command_build*, bld_data*);
int command_build_linker(bld_command_build*, bld_data*);
int command_build_apply_build(bld_command_build*, bld_data*);

int command_build(bld_command_build* build, bld_data* data) {
    switch (build->type) {
        case (BLD_COMMAND_BUILD_TARGET):
            return command_build_target(build, data);
        case (BLD_COMMAND_BUILD_COMPILER):
            return command_build_compiler(build, data);
        case (BLD_COMMAND_BUILD_LINKER):
            return command_build_linker(build, data);
    }
    log_fatal("command_build: unreachable error");
    return -1;
}

int command_build_target(bld_command_build* build, bld_data* data) {
    int result;
    bld_forward_project fproject;
    bld_project project;
    bld_compiler temp_c = compiler_copy(&data->target_config.files.info.compiler.as.compiler);
    bld_linker temp_l = linker_copy(&data->target_config.linker);
    bld_path path_cache, path_root;
    bld_string name_executable;
    log_debug("Build target: \"%s\"", string_unpack(&build->target));

    path_root = path_copy(&data->root);
    fproject = project_forward_new(&path_root, &temp_c, &temp_l);

    path_cache = path_from_string(".bld");
    path_append_string(&path_cache, "target");
    path_append_string(&path_cache, string_unpack(&build->target));
    path_append_string(&path_cache, "cache");
    log_debug("Path to cache: \"%s\"", path_to_string(&path_cache));
    project_load_cache(&fproject, path_to_string(&path_cache));

    log_debug("Main file: \"%s\"", path_to_string(&data->target_config.path_main));
    project_set_main_file(&fproject, path_to_string(&data->target_config.path_main));

    project_ignore_path(&fproject, "bld_core/test");  /* Ignore correctly */

    project = project_resolve(&fproject);

    log_error("APPLY CONFIGURATION");

    name_executable = string_copy(&build->target);
    string_append_string(&name_executable, "." BLD_EXECUTABLE_FILE_ENDING);
    result = incremental_compile_project(&project, string_unpack(&name_executable));

    string_free(&name_executable);
    path_free(&path_cache);
    project_free(&project);
    return result;
}

int command_build_compiler(bld_command_build* build, bld_data* data) {
    log_debug("Compiler target: \"%s\"", string_unpack(&build->target));
    log_debug("Path: \"%s\"", path_to_string(&build->path));
    (void)(data);
    return -1;
}

int command_build_linker(bld_command_build* build, bld_data* data) {
    log_debug("Linker target: \"%s\"", string_unpack(&build->target));
    log_debug("Path: \"%s\"", path_to_string(&build->path));
    (void)(data);
    return -1;
}

int command_build_parse(bld_string* target, bld_args* args, bld_data* data, bld_command_build* build, bld_command_invalid* invalid) {
    bld_string option, path;
    bld_string error_msg;
    bld_string compiler_option = string_pack("-c");
    bld_string linker_option = string_pack("-l");
    (void)(data);

    if (args_empty(args)) {
        build->type = BLD_COMMAND_BUILD_TARGET;
        build->target = string_copy(target);

        if (!data->target_config_parsed) {
            error_msg = string_pack("bld: no target config parsed");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        } else if (!data->target_config.files.info.compiler_set) {
            error_msg = string_pack("bld: target has no base compiler");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        } else if (data->target_config.files.info.compiler.type != BLD_COMPILER) {
            error_msg = string_pack("bld: target has no base compiler, only compiler flags");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        }

        return 0;
    }

    option = args_advance(args);
    if (string_eq(&option, &compiler_option)) {
        build->type = BLD_COMMAND_BUILD_COMPILER;
    } else if (string_eq(&option, &linker_option)) {
        build->type = BLD_COMMAND_BUILD_LINKER;
    } else {
        error_msg = string_new();
        string_append_string(&error_msg, "bld: cannot build target '");
        string_append_string(&error_msg, string_unpack(target));
        string_append_string(&error_msg, "', unknown option '");
        string_append_string(&error_msg, string_unpack(&option));
        string_append_string(&error_msg, "'");
        invalid->code = -1;
        invalid->msg = error_msg;
        return -1;
    }

    if (args_empty(args)) {
        error_msg = string_new();
        string_append_string(&error_msg, "bld: expected path");
        invalid->code = -1;
        invalid->msg = error_msg;
        return -1;
    }

    path = args_advance(args);
    build->target = string_copy(target);
    build->path = path_from_string(string_unpack(&path));
    return 0;
}

void command_build_free(bld_command_build* build) {
    string_free(&build->target);
    if (build->type != BLD_COMMAND_BUILD_TARGET) {
        path_free(&build->path);
    }
}
