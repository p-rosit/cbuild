#include "../bld_core/logging.h"
#include "../bld_core/iter.h"
#include "help.h"

const bld_string bld_command_string_help = STRING_COMPILE_TIME_PACK("help");
int command_help_command(bld_command_type, bld_data*);

int command_help(bld_command_help* help, bld_data* data) {
    bld_iter iter;
    bld_command_type* type;
    bld_handle_annotated* handle;

    if (help->has_command) {
        int found;
        bld_command_type result_type;

        found = 0;
        iter = iter_array(&data->handle_order);
        while (iter_next(&iter, (void**) &type)) {
            if (*type == BLD_COMMAND_INVALID) {continue;}

            handle = set_get(&data->handles, *type);
            if (handle == NULL) {log_fatal(LOG_FATAL_PREFIX "internal error");}

            if (string_eq(&help->command, &handle->name)) {
                found = 1;
                result_type = *type;
            }
        }

        if (!found) {
            printf("'%s' is not a command.\n", string_unpack(&help->command));
            return -1;
        }

        return command_help_command(result_type, data);
    } else {
        printf("Available sub commands:\n");

        iter = iter_array(&data->handle_order);
        while (iter_next(&iter, (void**) &type)) {
            if (*type == BLD_COMMAND_INVALID || *type == BLD_COMMAND_BUILD) {continue;}

            handle = set_get(&data->handles, *type);
            if (handle == NULL) {log_fatal(LOG_FATAL_PREFIX "internal error");}

            printf("    %s\n", string_unpack(&handle->name));
        }

        printf(
            "\n"
            "To see more information on each command separtely run\n"
            "\n"
            "    bld help <command name>\n"
            "\n"
            "To run a build command run `bld <target name>` or\n"
            "\n"
            "    bld help build\n"
            "\n"
            "for more information on building.\n"
        );
    }
    return 0;
}

int command_help_command(bld_command_type type, bld_data* data) {
    bld_handle_annotated* handle;
    bld_string help;

    handle = set_get(&data->handles, type);
    help = handle_make_description(&handle->handle);

    printf("%s\n", string_unpack(&help));
    string_free(&help);
    return 0;
}

int command_help_convert(bld_command* pre_cmd, bld_data* data, bld_command_help* cmd, bld_command_invalid* invalid) {
    bld_command_positional* arg;
    bld_command_positional_optional* opt;
    (void)(data);
    (void)(invalid);

    arg = array_get(&pre_cmd->positional, 1);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("missing optional argument");}
    opt = &arg->as.opt;

    cmd->has_command = opt->present;
    if (opt->present) {
        cmd->command = string_copy(&opt->value);
    }

    return 0;
}

bld_handle_annotated command_handle_help(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_HELP;
    handle.name = bld_command_string_help;
    handle.handle = handle_new(name);
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_help));
    handle_positional_optional(&handle.handle, "the command or target to display help text for");
    handle_set_description(&handle.handle, "A help command!");

    handle.convert = (bld_command_convert*) command_help_convert;
    handle.execute = (bld_command_execute*) command_help;
    handle.free = (bld_command_free*) command_help_free;

    return handle;
}

void command_help_free(bld_command_help* help) {
    if (help->has_command) {
        string_free(&help->command);
    }
}
