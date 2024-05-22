#include "bld_core/args.h"
#include "bld_core/logging.h"
#include "bld_core/path.h"
#include "command/command.h"

int main(int argc, char** argv) {
    int result;
    bld_args args;
    bld_data data;
    bld_application_command cmd;
    bld_set cmds;

    args = args_new(argc, argv);
    if (args_empty(&args)) {log_fatal("main: unreachable error");}
    args_advance(&args); /* Ignore how program was invoked */

    if (cmds.size <= 0) {log_fatal("No commands registered!");}
    data = data_extract("bld");
    cmd = application_command_parse(&args, &data);
    result = application_command_execute(&cmd, &data);

    application_command_free(&cmd, &data);
    data_free(&data);
    return result;
}
