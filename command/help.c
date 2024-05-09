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

int command_help(bld_command_help* help, bld_data* data) {
    (void)(data);
    if (help->has_command) {
        log_info("'%s' help is on the way", string_unpack(&help->command));
    } else {
        log_info("general help is on the way");
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

int command_help_parse(bld_args* args, bld_data* data, bld_command_help* help, bld_command_invalid* invalid) {
    bld_string command;
    (void)(data);
    (void)(invalid);

    if (args_empty(args)) {
        help->has_command = 0;
        return 0;
    }

    command = args_advance(args);
    help->has_command = 1;
    help->command = string_copy(&command);
    return 0;
}

void command_help_free(bld_command_help* help) {
    if (help->has_command) {
        string_free(&help->command);
    }
}
