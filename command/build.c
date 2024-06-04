#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "../bld_core/project.h"
#include "../bld_core/incremental.h"
#include "build.h"

int command_build_verify_config(bld_command_build*, bld_data*);
void command_build_apply_config(bld_forward_project* ,bld_command_build*, bld_data*);
void command_build_apply_build_info(bld_forward_project*, bld_path*, bld_target_build_information*);

int command_build(bld_command_build* cmd, bld_data* data) {
    int result;
    bld_forward_project fproject;
    bld_project project;
    bld_compiler temp_c;
    bld_linker temp_l;
    bld_path path_cache, path_root;
    bld_string name_executable;

    if (!set_has(&data->targets, string_hash(string_unpack(&cmd->target)))) {
        log_fatal("'%s' is not a target", string_unpack(&cmd->target));
        return -1; /* Unreachable */
    }

    config_target_load(data, &cmd->target);
    if (command_build_verify_config(cmd, data)) {
        return -1;
    }

    log_debug("Building target: \"%s\"", string_unpack(&cmd->target));
    temp_c = compiler_copy(&data->target_config.files.info.compiler.as.compiler);
    temp_l = linker_copy(&data->target_config.linker);

    path_root = path_copy(&data->root);
    fproject = project_forward_new(&path_root, &temp_c, &temp_l);

    path_cache = path_from_string(".bld");
    path_append_string(&path_cache, "target");
    path_append_string(&path_cache, string_unpack(&cmd->target));
    path_append_string(&path_cache, "cache");
    log_debug("Path to cache: \"%s\"", path_to_string(&path_cache));
    project_load_cache(&fproject, path_to_string(&path_cache));

    log_debug("Main file: \"%s\"", path_to_string(&data->target_config.path_main));
    project_set_main_file(&fproject, path_to_string(&data->target_config.path_main));

    command_build_apply_config(&fproject, cmd, data);

    project = project_resolve(&fproject);

    name_executable = string_copy(&cmd->target);
    string_append_string(&name_executable, "." BLD_EXECUTABLE_FILE_ENDING);
    result = incremental_compile_project(&project, string_unpack(&name_executable));

    project_save_cache(&project);

    string_free(&name_executable);
    path_free(&path_cache);
    project_free(&project);

    if (result < 0) {result = 0;}
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

    if (!utils_get_target(&cmd->target, &err, opt, data)) {
        error = -1;
        goto parse_failed;
    }

    return 0;
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
    bld_path* path, temp;
    bld_target_build_information* child;
    (void)(cmd);

    iter = iter_array(&data->target_config.added_paths);
    while (iter_next(&iter, (void**) &path)) {
        log_warn("Adding: %s", path_to_string(path));
        project_add_path(fproject, path_to_string(path));
    }

    iter = iter_array(&data->target_config.ignore_paths);
    while (iter_next(&iter, (void**) &path)) {
        log_warn("Ignoring: %s", path_to_string(path));
        project_ignore_path(fproject, path_to_string(path));
    }

    temp = path_from_string(".");
    iter = iter_array(&data->target_config.files.files);
    while (iter_next(&iter, (void**) &child)) {
        command_build_apply_build_info(fproject, &temp, child);
    }

    path_free(&temp);
}

void command_build_apply_build_info(bld_forward_project* fproject, bld_path* path, bld_target_build_information* info) {
    bld_iter iter;
    bld_target_build_information* child;
    bld_path sub_path;

    sub_path = path_copy(path);
    path_append_string(&sub_path, string_unpack(&info->name));

    if (info->info.compiler_set) {
        log_error("Set compiler: %s", path_to_string(&sub_path));
        if (info->info.compiler.type == BLD_COMPILER) {
            project_set_compiler(fproject, path_to_string(&sub_path), info->info.compiler.as.compiler);
        } else {
            project_set_compiler_flags(fproject, path_to_string(&sub_path), info->info.compiler.as.flags);
        }
    }

    if (info->info.linker_set) {
        log_error("Set linker: %s", path_to_string(&sub_path));
        project_set_linker_flags(fproject, path_to_string(&sub_path), info->info.linker_flags);
    }

    iter = iter_array(&info->files);
    while (iter_next(&iter, (void**) &child)) {
        command_build_apply_build_info(fproject, &sub_path, child);
    }

    path_free(&sub_path);
}

int command_build_verify_config(bld_command_build* cmd, bld_data* data) {
    int error = 0;
    bld_iter iter;
    bld_path* path;

    if (!data->target_config_parsed) {
        log_error("Config for target '%s' has not been parsed", string_unpack(&cmd->target));
        return -1;
    }

    if (!data->target_config.files_set) {
        log_error("target file config has not been set up");
        return -1;
    } else if (!data->target_config.files.info.compiler_set) {
        log_error("target has no base compiler");
        return -1;
    } else if (data->target_config.files.info.compiler.type != BLD_COMPILER) {
        log_error("target has no base compiler, only compiler flags");
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
