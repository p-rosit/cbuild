#include "../bld_core/os.h"
#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "add.h"

const bld_string bld_command_string_add = STRING_COMPILE_TIME_PACK("add");
const bld_string bld_command_string_add_terse_delete = STRING_COMPILE_TIME_PACK("-d");
const bld_string bld_command_string_add_flag_delete = STRING_COMPILE_TIME_PACK("--delete");

int command_add(bld_command_add* cmd, bld_data* data) {
    bld_iter iter;
    bld_config_target* config = &data->target_config;
    bld_path path, *temp;
    bld_set added_files = set_new(sizeof(bld_path));
    uintmax_t added_id;
    if (!data->target_config_parsed) {
        log_fatal("Could not parse config of target \"%s\"", string_unpack(&cmd->target));
    }

    iter = iter_array(&config->added_paths);
    while (iter_next(&iter, (void**) &temp)) {
        added_id = os_info_id(path_to_string(temp));
        if (added_id == BLD_INVALID_IDENITIFIER) {continue;}
        set_add(&added_files, added_id, temp);
    }

    added_id = os_info_id(path_to_string(&cmd->path));

    if (added_id == BLD_INVALID_IDENITIFIER) {
        set_free(&added_files);
        log_error("Attempting to add '%s' which is not a valid path to a file/directory", path_to_string(&cmd->path));
        return -1;
    }

    if (cmd->remove_flag) {
        bld_iter iter;
        bld_string a;
        bld_path *path, *temp;
        size_t index;

        path = set_get(&added_files, added_id);
        if (path == NULL) {
            set_free(&added_files);
            log_error("Attempting to remove '%s' which has not been added to target '%s'", path_to_string(&cmd->path), string_unpack(&cmd->target));
            return -1;
        }

        index = 0;
        a = string_pack(path_to_string(path));
        iter = iter_array(&data->target_config.added_paths);
        while (iter_next(&iter, (void**) &temp)) {
            bld_string b = string_pack(path_to_string(temp));
            if (string_eq(&a, &b)) {
                break;
            }
            index += 1;
        }

        path = array_get(&data->target_config.added_paths, index);
        path_free(path);
        array_remove(&data->target_config.added_paths, index);

        log_info("Removed: \"%s\" for target \"%s\"", path_to_string(&cmd->path), string_unpack(&cmd->target));
    } else {
        if (set_has(&added_files, added_id)) {
            set_free(&added_files);
            log_error("Attempting to add '%s' which has already been added by to '%s'", path_to_string(&cmd->path), string_unpack(&cmd->target));
            return -1;
        }

        log_info("Adding: \"%s\" for target \"%s\"", path_to_string(&cmd->path), string_unpack(&cmd->target));
        path = path_copy(&cmd->path);
        array_push(&data->target_config.added_paths, &path);
    }

    config_target_save(data, &cmd->target, config);

    set_free(&added_files);
    return 0;
}

int command_add_parse(bld_string* target, bld_args* args, bld_data* data, bld_command_add* cmd, bld_command_invalid* invalid) {
    bld_string path_add, error_msg;
    (void)(data);

    if (args_empty(args)) {
        error_msg = string_pack("bld: missing path");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    path_add = args_advance(args);

    if (args_empty(args)) {
        cmd->remove_flag = 0;
    } else {
        bld_string temp = args_advance(args);
        cmd->remove_flag = 1;

        if (!string_eq(&path_add, &bld_command_string_add_terse_delete) && !string_eq(&path_add, &bld_command_string_add_flag_delete)) {
            error_msg = string_pack("bld: invalid arguments");
            invalid->code = -1;
            invalid->msg = string_copy(&error_msg);
            return -1;
        }

        path_add = temp;
    }

    if (!args_empty(args)) {
        error_msg = string_pack("bld: too many inputs");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    cmd->target = string_copy(target);
    cmd->path = path_from_string(string_unpack(&path_add));
    return 0;
}

void command_add_free(bld_command_add* add) {
    string_free(&add->target);
    path_free(&add->path);
}
