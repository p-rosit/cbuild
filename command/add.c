#include "../bld_core/os.h"
#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "init.h"
#include "add.h"

const bld_string bld_command_string_add = STRING_COMPILE_TIME_PACK("add");
const bld_string bld_command_string_add_flag_delete = STRING_COMPILE_TIME_PACK("delete");
const bld_string bld_command_add_see_more = STRING_COMPILE_TIME_PACK(
    "See `bld help add` for more information."
);

int command_add(bld_command_add* cmd, bld_data* data) {
    bld_iter iter;
    bld_config_target* config;
    bld_path path, *temp;
    bld_set added_files;
    uintmax_t added_id;

    if (!set_has(&data->targets, string_hash(string_unpack(&cmd->target)))) {
        printf("'%s' is not a known target. %s\n", string_unpack(&cmd->target), string_unpack(&bld_command_add_see_more));
        return -1;
    }

    config = &data->target_config;
    added_files = set_new(sizeof(bld_path));
    config_target_load(data, &cmd->target);
    if (!data->target_config_parsed) {log_fatal("Could not parse config of target \"%s\"", string_unpack(&cmd->target));}
    log_info("Chosen target: \"%s\"", string_unpack(&cmd->target));

    iter = iter_array(&config->added_paths);
    while (iter_next(&iter, (void**) &temp)) {
        added_id = os_info_id(path_to_string(temp));
        if (added_id == BLD_INVALID_IDENITIFIER) {continue;}
        set_add(&added_files, added_id, temp);
    }

    iter = iter_array(&cmd->paths);
    while (iter_next(&iter, (void**) &temp)) {
        added_id = os_info_id(path_to_string(temp));

        if (added_id == BLD_INVALID_IDENITIFIER) {
            set_free(&added_files);
            log_error("Attempting to add '%s' which is not a valid path to a file/directory", path_to_string(temp));
            return -1;
        }

        if (!cmd->remove_flag) {
            if (set_has(&added_files, added_id)) {
                set_free(&added_files);
                log_error("Attempting to add '%s' which has already been added to '%s'", path_to_string(temp), string_unpack(&cmd->target));
                return -1;
            }

            log_info("Adding: \"%s\"", path_to_string(temp));
            path = path_copy(temp);
            array_push(&data->target_config.added_paths, &path);
        } else {
            bld_iter iter;
            bld_string a;
            bld_path *path, *p;
            size_t index;

            path = set_get(&added_files, added_id);
            if (path == NULL) {
                set_free(&added_files);
                log_error("Attempting to remove '%s' which has not been added to target '%s'", path_to_string(temp), string_unpack(&cmd->target));
                return -1;
            }

            index = 0;
            a = string_pack(path_to_string(path));
            iter = iter_array(&data->target_config.added_paths);
            while (iter_next(&iter, (void**) &p)) {
                bld_string b;

                b = string_pack(path_to_string(p));
                if (string_eq(&a, &b)) {break;}
                index += 1;
            }

            path = array_get(&data->target_config.added_paths, index);
            path_free(path);
            array_remove(&data->target_config.added_paths, index);

            log_info("Removed: \"%s\"", path_to_string(temp));
        }
    }

    config_target_save(data, &cmd->target);
    set_free(&added_files);
    return 0;
}

int command_add_convert(bld_command* pre_cmd, bld_data* data, bld_command_add* cmd, bld_command_invalid* invalid) {
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
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_add_convert: missing first optional");}
    opt = &arg->as.opt;

    if (!utils_get_target(&cmd->target, &err, opt, data)) {
        error = -1;
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 2);
    if (arg->type != BLD_HANDLE_POSITIONAL_VARGS) {log_fatal("command_add_convert: no varg");}
    varg = &arg->as.vargs;

    if (varg->values.size <= 0) {
        error = -1;
        err = string_pack("expected paths to add\n");
        err = string_copy(&err);
        goto free_target;
    }

    cmd->paths = array_new(sizeof(bld_path));
    iter = iter_array(&varg->values);
    while (iter_next(&iter, (void**) &str_path)) {
        path = path_from_string(string_unpack(str_path));
        array_push(&cmd->paths, &path);
    }

    cmd->remove_flag = set_has(&pre_cmd->flags, string_hash(string_unpack(&bld_command_string_add_flag_delete)));

    return 0;
    free_target:
    string_free(&cmd->target);
    parse_failed:
    *invalid = command_invalid_new(error, &err);
    return -1;
}

bld_handle_annotated command_handle_add(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_ADD;
    handle.name = bld_command_string_add;
    handle.handle = handle_new(name);
    handle_positional_optional(&handle.handle, "The target to modify");
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_add));
    handle_allow_flags(&handle.handle);
    handle_positional_vargs(&handle.handle, "The paths to add to the chosen target");
    handle_flag(&handle.handle, *bld_command_string_add_flag_delete.chars, string_unpack(&bld_command_string_add_flag_delete), "Remove the specified paths from paths added to target");
    handle_set_description(&handle.handle, "Adds the specified paths to the specified target. If no target is supplied\nthe paths will be added to the currently active target.");

    handle.convert = (bld_command_convert*) command_add_convert;
    handle.execute = (bld_command_execute*) command_add;
    handle.free = (bld_command_free*) command_add_free;

    return handle;
}

void command_add_free(bld_command_add* cmd) {
    bld_iter iter;
    bld_path* path;

    string_free(&cmd->target);

    iter = iter_array(&cmd->paths);
    while (iter_next(&iter, (void**) &path)) {
        path_free(path);
    }
    array_free(&cmd->paths);
}
