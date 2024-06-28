#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "../bld_core/iter.h"
#include "init.h"
#include "ignore.h"

const bld_string bld_command_string_ignore = STRING_COMPILE_TIME_PACK("ignore");
const bld_string bld_command_string_ignore_flag_delete = STRING_COMPILE_TIME_PACK("delete");

int command_ignore(bld_command_ignore* cmd, bld_data* data) {
    bld_iter iter;
    bld_config_target* config;
    bld_path path, *temp;
    bld_set ignored_files;
    uintmax_t added_id;

    ignored_files = set_new(sizeof(bld_path));
    config = &data->target_config;
    config_target_load(data, &cmd->target);
    if (!data->target_config_parsed) {log_fatal("Could not parse config of target \"%s\"", string_unpack(&cmd->target));}

    log_info("Chosen target: \"%s\"", string_unpack(&cmd->target));

    iter = iter_array(&config->ignore_paths);
    while (iter_next(&iter, (void**) &temp)) {
        added_id = os_info_id(path_to_string(temp));
        if (added_id == BLD_INVALID_IDENITIFIER) {continue;}
        set_add(&ignored_files, added_id, temp);
    }

    iter = iter_array(&cmd->paths);
    while (iter_next(&iter, (void**) &temp)) {
        added_id = os_info_id(path_to_string(temp));

        if (added_id == BLD_INVALID_IDENITIFIER) {
            set_free(&ignored_files);
            log_error("Attempting to ignore '%s' which is not a valid path to a file/directory", path_to_string(temp));
            return -1;
        }

        if (!cmd->remove_flag) {
            if (set_has(&ignored_files, added_id)) {
                set_free(&ignored_files);
                log_error("Attempting to ignore '%s' is already being ignored by '%s'", path_to_string(temp), string_unpack(&cmd->target));
                return -1;
            }

            log_info("Ignoring: \"%s\"", path_to_string(temp));
            path = path_copy(temp);
            array_push(&data->target_config.ignore_paths, &path);
        } else {
            bld_iter iter;
            bld_string a;
            bld_path *path, *p;
            size_t index;

            path = set_get(&ignored_files, added_id);
            if (path == NULL) {
                set_free(&ignored_files);
                log_error("Attempting to remove '%s' which is not being ignored by target '%s'", path_to_string(temp), string_unpack(&cmd->target));
                return -1;
            }

            index = 0;
            a = string_pack(path_to_string(path));
            iter = iter_array(&data->target_config.ignore_paths);
            while (iter_next(&iter, (void**) &p)) {
                bld_string b = string_pack(path_to_string(p));
                if (string_eq(&a, &b)) {break;}
                index += 1;
            }

            path = array_get(&data->target_config.ignore_paths, index);
            path_free(path);
            array_remove(&data->target_config.ignore_paths, index);

            log_info("Removed: \"%s\"", path_to_string(temp));
        }
    }

    config_target_save(data, &cmd->target);
    set_free(&ignored_files);
    return 0;
}

int command_ignore_convert(bld_command* pre_cmd, bld_data* data, bld_command_ignore* cmd, bld_command_invalid* invalid) {
    int error;
    bld_iter iter;
    bld_string err;
    bld_string *str_path;
    bld_path path;
    bld_command_positional* arg;
    bld_command_positional_optional* opt;
    bld_command_positional_vargs* varg;

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
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_ignore_convert: missing first optional");}
    opt = &arg->as.opt;

    if (!utils_get_target(&cmd->target, &err, opt, data)) {
        error = -1;
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 2);
    if (arg->type != BLD_HANDLE_POSITIONAL_VARGS) {log_fatal("command_ignore_convert: no varg");}
    varg = &arg->as.vargs;

    if (varg->values.size <= 0) {
        error = -1;
        err = string_pack("expected paths to ignore\n");
        err = string_copy(&err);
        *invalid = command_invalid_new(-1, &err);
        goto free_target;
    }

    cmd->paths = array_new(sizeof(bld_path));
    iter = iter_array(&varg->values);
    while (iter_next(&iter, (void**) &str_path)) {
        path = path_from_string(string_unpack(str_path));
        array_push(&cmd->paths, &path);
    }

    cmd->remove_flag = set_has(&pre_cmd->flags, string_hash(string_unpack(&bld_command_string_ignore_flag_delete)));

    return 0;
    free_target:
    string_free(&cmd->target);
    parse_failed:
    *invalid = command_invalid_new(error, &err);
    return -1;
}

bld_handle_annotated command_handle_ignore(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_IGNORE;
    handle.name = bld_command_string_ignore;
    handle.handle = handle_new(name);
    handle_positional_optional(&handle.handle, "The target to modify.");
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_ignore));
    handle_allow_flags(&handle.handle);
    handle_positional_vargs(&handle.handle, "The paths to ignore on the chosen the target");
    handle_flag(&handle.handle, *bld_command_string_ignore_flag_delete.chars, string_unpack(&bld_command_string_ignore_flag_delete), "Remove the specified paths from the ignored paths of the target");
    handle_set_description(&handle.handle, "Ignores the specified paths in the specified target. If no target is supplied\nthe paths will be ignored by the currently active target.");

    handle.convert = (bld_command_convert*) command_ignore_convert;
    handle.execute = (bld_command_execute*) command_ignore;
    handle.free = (bld_command_free*) command_ignore_free;

    return handle;
}

void command_ignore_free(bld_command_ignore* cmd) {
    bld_iter iter;
    bld_path* path;

    string_free(&cmd->target);

    iter = iter_array(&cmd->paths);
    while (iter_next(&iter, (void**) &path)) {
        path_free(path);
    }
    array_free(&cmd->paths);
}
