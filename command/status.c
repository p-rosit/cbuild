#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "init.h"
#include "status.h"

const bld_string bld_command_string_status = STRING_COMPILE_TIME_PACK("status");
const bld_string bld_command_string_status_terse_active = STRING_COMPILE_TIME_PACK("-a");
const bld_string bld_command_string_status_flag_active = STRING_COMPILE_TIME_PACK("--active");

int command_status_all(bld_command_status*, bld_data*);
int command_status_target(bld_command_status*, bld_data*);

int command_status(bld_command_status* status, bld_data* data) {
    if (!data->has_root) {
        printf("%s", string_unpack(&bld_command_init_missing_project));
        return -1;
    }

    set_log_level(data->config.log_level);

    if (data->targets.size == 0) {
        printf("%s", string_unpack(&bld_command_init_no_targets));
        return -1;
    }

    if (status->target_status) {
        return command_status_target(status, data);
    } else {
        return command_status_all(status, data);
    }
}

int command_status_all(bld_command_status* status, bld_data* data) {
    bld_iter iter;
    bld_string* target;
    (void)(status);

    printf("Targets in project:\n");

    iter = iter_set(&data->targets);
    while (iter_next(&iter, (void**) &target)) {
        int is_default = 0;
        if (data->config.active_target_configured) {
            if (string_eq(target, &data->config.active_target)) {
                is_default = 1;
            }
        }

        if (is_default) {
            printf("  * %s\n", string_unpack(target));
        } else {
            printf("    %s\n", string_unpack(target));
        }
    }
    return 0;
}

int command_status_target(bld_command_status* status, bld_data* data) {
    bld_config_target* config;
    bld_iter iter;
    bld_path* path;
    log_info("Target: %s", string_unpack(&status->target));

    config = &data->target_config;
    config_target_load(data, &status->target);
    if (!data->target_config_parsed) {
        log_fatal("Target config has not been parsed");
    }

    if (config->added_paths.size > 0) {
        log_info("Added:");
        iter = iter_array(&data->target_config.added_paths);
        while (iter_next(&iter, (void**) &path)) {
            log_info("  %s", path_to_string(path));
        }
    }

    if (config->ignore_paths.size > 0) {
        log_info("Ignored:");
        iter = iter_array(&data->target_config.ignore_paths);
        while (iter_next(&iter, (void**) &path)) {
            log_info("  %s", path_to_string(path));
        }
    }
    return 0;
}

int command_status_convert(bld_command* pre_cmd, bld_data* data, bld_command_status* cmd, bld_command_invalid* invalid) {
    bld_command_positional* arg;
    bld_command_positional_optional* target;
    (void)(data);
    (void)(invalid);

    arg = array_get(&pre_cmd->positional, 1);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_status_convert: missing first optional");}
    target = &arg->as.opt;

    cmd->target_status = target->present;
    if (target->present) {
        cmd->target = string_copy(&target->value);
    }

    return 0;
}

bld_handle_annotated command_handle_status(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_STATUS;
    handle.name = bld_command_string_status;
    handle.handle = handle_new(name);
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_status));
    handle_positional_optional(&handle.handle, "The target to show the status of");
    handle_set_description(
        &handle.handle,
        "If no target is supplied the list of existing targets will be printed\n"
        "as well as which target, if selected, is active. See `bld help switch`\n"
        "for more information about active targets.\n"
        "\n"
        "If a target is supplied the status of that target will be printed."
    );

    handle.convert = (bld_command_convert*) command_status_convert;
    handle.execute = (bld_command_execute*) command_status;
    handle.free = (bld_command_free*) command_status_free;

    return handle;
}

void command_status_free(bld_command_status* status) {
    if (status->target_status) {
        string_free(&status->target);
    }
}
