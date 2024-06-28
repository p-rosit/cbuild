#include "../bld_core/logging.h"
#include "../bld_core/iter.h"
#include "../config/config_target.h"
#include "command.h"
#include "handle.h"

bld_application_command application_command_parse(bld_args* args, bld_data* data) {
    int error, matched;
    bld_array errs;
    bld_iter iter;
    bld_handle_annotated* handle;
    bld_command_type* handle_id;
    bld_command cmd;
    bld_application_command app_command;
    bld_command_invalid invalid;

    matched = 0;
    iter = iter_array(&data->handle_order);
    while (iter_next(&iter, (void**) &handle_id)) {
        bld_iter iter;
        bld_string* err;
        handle = set_get(&data->handles, *handle_id);
        if (handle == NULL) {log_fatal("Could not extract handle %d", handle_id);}
        if (handle->type == BLD_COMMAND_INVALID) {continue;}

        error = handle_parse(*args, &handle->handle, &cmd, &errs);

        if (handle->type == BLD_COMMAND_BUILD) {
            if (!(error & (BLD_COMMAND_ERROR_ARGS_TOO_FEW | BLD_COMMAND_ERROR_ARGS_TOO_MANY))) {
                matched = 1;
                break;
            }
        } else if (!(error & BLD_COMMAND_ERROR_ARGS_NO_MATCH)) {
            matched = 1;
            break;
        }

        iter = iter_array(&errs);
        while (iter_next(&iter, (void**) &err)) {
            string_free(err);
        }
        array_free(&errs);
    }

    if (!matched) {
        log_fatal("No subcommand could be matched");
    }

    if (error && !data->has_root) {
        bld_string* str;
        bld_string err;

        iter = iter_array(&errs);
        while (iter_next(&iter, (void**) &str)) {
            string_free(str);
        }
        array_free(&errs);

        err = string_copy(&bld_command_init_missing_project);

        app_command.type = BLD_COMMAND_INVALID;
        app_command.as.invalid = command_invalid_new(-1, &err);
    } else if (error) {
        bld_string* str;
        bld_string err;

        err = string_new();
        if (errs.size <= 0) {log_fatal("application_command_parse: error but no error");}

        iter = iter_array(&errs);
        while (iter_next(&iter, (void**) &str)) {
            string_append_string(&err, string_unpack(str));
            string_append_char(&err, '\n');
            string_free(str);
        }
        array_free(&errs);

        string_append_char(&err, '\n');
        string_append_string(&err, "For help with this subcommand see `bld help ");
        string_append_string(&err, string_unpack(&handle->name));
        string_append_string(&err, "'\n");
        
        app_command.type = BLD_COMMAND_INVALID;
        app_command.as.invalid = command_invalid_new(-1, &err);
    } else {
        app_command.type = handle->type;
        error = handle->convert(&cmd, data, &app_command.as, &invalid);
        command_free(&cmd);

        if (error) {
            app_command.type = BLD_COMMAND_INVALID;
            app_command.as.invalid = invalid;
        }
    }

    return app_command;
}

int application_command_execute(bld_application_command* cmd, bld_data* data) {
    bld_handle_annotated* handle;

    handle = set_get(&data->handles, cmd->type);
    if (handle == NULL) {log_fatal("application_command_execute: no handle");}

    return handle->execute(&cmd->as, data);
}

void application_command_free(bld_application_command* cmd, bld_data* data) {
    bld_handle_annotated* handle;

    handle = set_get(&data->handles, cmd->type);
    if (handle == NULL) {log_fatal("application_command_free: no handle");}

    handle->free(&cmd->as);
}
