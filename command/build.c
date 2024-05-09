#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "../bld_core/project.h"
#include "../bld_core/incremental.h"
#include "build.h"
int command_build_apply_build(bld_command_build*, bld_data*);

int command_build(bld_command_build* build, bld_data* data) {
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


int command_build_parse(bld_string* target, bld_args* args, bld_data* data, bld_command_build* build, bld_command_invalid* invalid) {
    bld_string error_msg;

    if (!args_empty(args)) {
        error_msg = string_pack("bld: too many input arguments");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    if (!data->target_config_parsed) {
        error_msg = string_pack("bld: no target config parsed");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    } else if (!data->target_config.files_set) {
        error_msg = string_pack("bld: target file config has not been set up");
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

    build->target = string_copy(target);
    return 0;
}

void command_build_free(bld_command_build* build) {
    string_free(&build->target);
}
