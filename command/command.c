#include "../bld_core/logging.h"
#include "../bld_core/iter.h"
#include "../config/config_target.h"
#include "command.h"
#include "handle.h"

typedef int (bld_command_convert)(bld_command*, bld_data*, void*, bld_command_invalid*);
typedef int (bld_command_execute)(void*, bld_data*);
typedef void (bld_command_free)(void*);

typedef struct bld_handle_named {
    bld_command_type type;
    bld_handle handle;
    bld_command_convert* convert;
    bld_command_execute* execute;
    bld_command_free* free;
} bld_handle_named;

bld_set application_available_set(char* name) {
    bld_set cmds = set_new(sizeof(bld_handle_named));
    bld_handle_named handle;

    handle.type = BLD_COMMAND_ADD;
    handle.handle = command_handle_add(name);
    handle.convert = (bld_command_convert*) command_add_convert;
    handle.execute = (bld_command_execute*) command_add;
    handle.free = (bld_command_free*) command_add_free;
    set_add(&cmds, BLD_COMMAND_ADD, &handle);

    handle.type = BLD_COMMAND_INVALID;
    handle.execute = (bld_command_execute*) command_invalid;
    handle.free = (bld_command_free*) command_invalid_free;
    set_add(&cmds, BLD_COMMAND_INVALID, &handle);

    return cmds;
}

void application_available_free(bld_set* cmds) {
    bld_iter iter = iter_set(cmds);
    bld_handle_named* handle;
    while (iter_next(&iter, (void**) &handle)) {
        if (handle->type == BLD_COMMAND_INVALID) {continue;}
        handle_free(&handle->handle);
    }
    set_free(cmds);
}

bld_application_command application_command_parse(bld_args* args, bld_data* data, bld_set* handles) {
    int error;
    bld_array errs;
    bld_iter iter = iter_set(handles);
    bld_handle_named* handle;
    bld_command cmd;
    bld_application_command app_command;
    bld_command_invalid invalid;
    (void)(data);

    while (iter_next(&iter, (void**) &handle)) {
        bld_iter iter;
        bld_string* err;
        if (handle->type == BLD_COMMAND_INVALID) {continue;}

        error = handle_parse(*args, &handle->handle, &cmd, &errs);
        if (!(error & BLD_COMMAND_ERROR_ARGS_NO_MATCH)) {
            break;
        }

        iter = iter_array(&errs);
        while (iter_next(&iter, (void**) &err)) {
            string_free(err);
        }
        array_free(&errs);
    }

    if (error) {
        bld_string* str;
        bld_string err = string_new();

        if (errs.size <= 0) {log_fatal("application_command_parse: error but no error");}

        iter = iter_array(&errs);
        while (iter_next(&iter, (void**) &str)) {
            string_append_string(&err, string_unpack(str));
            string_append_char(&err, '\n');
            string_free(str);
        }
        array_free(&errs);
        
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

int application_command_execute(bld_application_command* cmd, bld_data* data, bld_set* commands) {
    bld_handle_named* handle = set_get(commands, cmd->type);
    if (handle == NULL) {log_fatal("application_command_execute: no handle");}
    return handle->execute(&cmd->as, data);
}

void application_command_free(bld_application_command* cmd, bld_set* commands) {
    bld_handle_named* handle = set_get(commands, cmd->type);
    if (handle == NULL) {log_fatal("application_command_free: no handle");}
    handle->free(&cmd->as);
}
