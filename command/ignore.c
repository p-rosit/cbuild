#include "../bld_core/os.h"
#include "../bld_core/logging.h"
#include "../bld_core/iter.h"
#include "ignore.h"

const bld_string bld_command_string_ignore = STRING_COMPILE_TIME_PACK("ignore");

int command_ignore(bld_command_ignore* cmd, bld_data* data) {
    bld_iter iter;
    bld_config_target* config = &data->target_config;
    bld_path path, *temp;
    bld_set ignored_files = set_new(0);
    uintmax_t ignore_id;
    if (!data->target_config_parsed) {
        log_fatal("Could not parse config of target \"%s\"", string_unpack(&cmd->target));
    }

    iter = iter_array(&config->ignore_paths);
    while (iter_next(&iter, (void**) &temp)) {
        ignore_id = os_info_id(path_to_string(temp));
        set_add(&ignored_files, ignore_id, NULL);
    }

    ignore_id = os_info_id(path_to_string(&cmd->path));

    if (ignore_id == BLD_INVALID_IDENITIFIER) {
        log_error("Requested to ignore '%s' which is not a valid path to a file/directory", path_to_string(&cmd->path));
        return -1;
    }

    if (set_has(&ignored_files, ignore_id)) {
        set_free(&ignored_files);
        log_error("Requested to ignore '%s' which is already being ignored by target '%s'", path_to_string(&cmd->path), string_unpack(&cmd->target));
        return -1;
    }

    log_info("Ignoring: \"%s\" for target \"%s\"", path_to_string(&cmd->path), string_unpack(&cmd->target));

    path = path_copy(&cmd->path);
    array_push(&config->ignore_paths, &path);
    config_target_save(data, &cmd->target, config);

    set_free(&ignored_files);
    return 0;
}

int command_ignore_parse(bld_string* target, bld_args* args, bld_data* data, bld_command_ignore* cmd, bld_command_invalid* invalid) {
    bld_string path_add, error_msg;
    (void)(data);

    if (args_empty(args)) {
        error_msg = string_pack("bld: missing path");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    path_add = args_advance(args);

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

void command_ignore_free(bld_command_ignore* cmd) {
    string_free(&cmd->target);
    path_free(&cmd->path);
}
