#include "../bld_core/logging.h"
#include "help.h"

#include "add.h"
#include "build.h"
#include "compiler.h"
#include "ignore.h"
#include "init.h"
#include "invalidate.h"
#include "linker.h"
#include "remove.h"
#include "status.h"
#include "switch.h"

const bld_string bld_command_string_help = STRING_COMPILE_TIME_PACK("help");
int command_help_command(bld_command_type, bld_data*);
int command_help_target(bld_string*, bld_data*);

int command_help(bld_command_help* help, bld_data* data) {
    (void)(data);
    if (help->has_command) {
        bld_command_type type;
        if (set_has(&data->targets, string_hash(string_unpack(&help->command)))) {
            return command_help_target(&help->command, data);
        } else if (string_eq(&help->command, &bld_command_string_add)) {
            type = BLD_COMMAND_ADD;
        } else if (string_eq(&help->command, &bld_command_string_compiler)) {
            type = BLD_COMMAND_COMPILER;
        } else if (string_eq(&help->command, &bld_command_string_help)) {
            type = BLD_COMMAND_HELP;
        } else if (string_eq(&help->command, &bld_command_string_ignore)) {
            type = BLD_COMMAND_IGNORE;
        } else if (string_eq(&help->command, &bld_command_string_init)) {
            type = BLD_COMMAND_INIT;
        } else if (string_eq(&help->command, &bld_command_string_invalidate)) {
            type = BLD_COMMAND_INVALIDATE;
        } else if (string_eq(&help->command, &bld_command_string_remove)) {
            type = BLD_COMMAND_REMOVE;
        } else if (string_eq(&help->command, &bld_command_string_status)) {
            type = BLD_COMMAND_STATUS;
        } else if (string_eq(&help->command, &bld_command_string_switch)) {
            type = BLD_COMMAND_SWITCH;
        } else {
            printf("'%s' is not a command or target\n", string_unpack(&help->command));
            return -1;
        }
        return command_help_command(type, data);
    } else {
        log_info("Available commands:");
        log_info("  %s", string_unpack(&bld_command_string_add));
        log_info("  %s", string_unpack(&bld_command_string_compiler));
        log_info("  %s", string_unpack(&bld_command_string_ignore));
        log_info("  %s", string_unpack(&bld_command_string_init));
        log_info("  %s", string_unpack(&bld_command_string_invalidate));
        log_info("  %s", string_unpack(&bld_command_string_linker));
        log_info("  %s", string_unpack(&bld_command_string_remove));
        log_info("  %s", string_unpack(&bld_command_string_status));
        log_info("  %s", string_unpack(&bld_command_string_switch));
    }
    return 0;
}

int command_help_command(bld_command_type type, bld_data* data) {
    bld_handle_annotated* handle = set_get(&data->handles, type);
    bld_string help = handle_make_description(&handle->handle);
    printf("%s\n", string_unpack(&help));
    string_free(&help);
    return 0;
}

int command_help_target(bld_string* target, bld_data* data) {
    log_warn("Help with target: \"%s\"\n", string_unpack(target));
    (void)(data);
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
    handle.handle = handle_new(name);
    handle_positional_expect(&handle.handle, "help");
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
