#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "../bld_core/project.h"
#include "../bld_core/incremental.h"
#include "build.h"

int command_build_verify_config(bld_command_build*, bld_data*);
void command_build_apply_config(bld_forward_project* ,bld_command_build*, bld_data*);

int command_build(bld_command_build* build, bld_data* data) {
    int result;
    bld_forward_project fproject;
    bld_project project;
    bld_compiler temp_c = compiler_copy(&data->target_config.files.info.compiler.as.compiler);
    bld_linker temp_l = linker_copy(&data->target_config.linker);
    bld_path path_cache, path_root;
    bld_string name_executable;
    log_debug("Building target: \"%s\"", string_unpack(&build->target));

    if (command_build_verify_config(build, data)) {
        compiler_free(&temp_c);
        linker_free(&temp_l);
        return -1;
    }

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

    command_build_apply_config(&fproject, build, data);

    project = project_resolve(&fproject);

    name_executable = string_copy(&build->target);
    string_append_string(&name_executable, "." BLD_EXECUTABLE_FILE_ENDING);
    result = incremental_compile_project(&project, string_unpack(&name_executable));

    project_save_cache(&project);

    string_free(&name_executable);
    path_free(&path_cache);
    project_free(&project);

    if (result < 0) {
        result = 0;
    }
    return result;
}


int command_build_convert(bld_command* pre_cmd, bld_data* data, bld_command_build* cmd, bld_command_invalid* invalid) {
    int error;
    bld_string err;
    bld_command_positional* arg;
    bld_command_positional_optional* opt;

    arg = array_get(&pre_cmd->positional, 0);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_build_convert: missing first optional");}
    opt = &arg->as.opt;

    if (opt->present) {
        cmd->target = string_copy(&opt->value);
    } else {
        if (!data->config.active_target_configured) {
            error = -1;
            err = string_pack("bld: building active target but no active target set.\n");
            goto parse_failed;
        }
        cmd->target = string_copy(&data->config.active_target);
    }

    data->target_config_parsed = !config_target_load(data, &cmd->target, &data->target_config);

    if (!data->target_config_parsed) {
        error = -1;
        err = string_pack("bld: no target config parsed\n");
        err = string_copy(&err);
        goto free_target;
    } else if (!data->target_config.files_set) {
        error = -1;
        err = string_pack("bld: target file config has not been set up\n");
        err = string_copy(&err);
        goto free_target;
    } else if (!data->target_config.files.info.compiler_set) {
        error = -1;
        err = string_pack("bld: target has no base compiler\n");
        err = string_copy(&err);
        goto free_target;
    } else if (data->target_config.files.info.compiler.type != BLD_COMPILER) {
        error = -1;
        err = string_pack("bld: target has no base compiler, only compiler flags\n");
        err = string_copy(&err);
        goto free_target;
    }

    return 0;
    free_target:
    string_free(&cmd->target);
    parse_failed:
    *invalid = command_invalid_new(error, &err);
    return -1;
}

bld_handle_annotated command_handle_build(char* name) {
    bld_handle_annotated handle;
    handle.handle = handle_new(name);
    handle_positional_optional(&handle.handle, "The target to build");
    handle_set_description(&handle.handle, "Build a target");

    handle.type = BLD_COMMAND_BUILD;
    handle.convert = (bld_command_convert*) command_build_convert;
    handle.execute = (bld_command_execute*) command_build;
    handle.free = (bld_command_free*) command_build_free;

    return handle;
}

void command_build_free(bld_command_build* build) {
    string_free(&build->target);
}

void command_build_apply_config(bld_forward_project* fproject, bld_command_build* cmd, bld_data* data) {
    bld_iter iter;
    bld_path* path;
    (void)(cmd);

    iter = iter_array(&data->target_config.added_paths);
    while (iter_next(&iter, (void**) &path)) {
        project_add_path(fproject, path_to_string(path));
    }

    iter = iter_array(&data->target_config.ignore_paths);
    while (iter_next(&iter, (void**) &path)) {
        project_ignore_path(fproject, path_to_string(path));
    }
}

int command_build_verify_config(bld_command_build* cmd, bld_data* data) {
    int error = 0;
    bld_iter iter;
    bld_path* path;

    if (!data->target_config_parsed) {
        log_error("Config for target '%s' has not been parsed", string_unpack(&cmd->target));
        return -1;
    }

    iter = iter_array(&data->target_config.added_paths);
    while (iter_next(&iter, (void**) &path)) {
        uintmax_t file_id = os_info_id(path_to_string(path));
        if (file_id == BLD_INVALID_IDENITIFIER) {
            error |= -1;
            log_error("Added path '%s' does not exist", path_to_string(path));
        }
    }

    iter = iter_array(&data->target_config.ignore_paths);
    while (iter_next(&iter, (void**) &path)) {
        uintmax_t file_id = os_info_id(path_to_string(path));
        if (file_id == BLD_INVALID_IDENITIFIER) {
            error |= -1;
            log_error("Ignored path '%s' does not exist", path_to_string(path));
        }
    }

    return error;
}
