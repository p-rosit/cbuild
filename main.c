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

    cmds = application_available_set("bld");
    if (cmds.size <= 0) {log_fatal("No commands registered!");}

    data = data_extract();
    cmd = application_command_parse(&args, &data, &cmds);
    result = application_command_execute(&cmd, &data, &cmds);

    application_command_free(&cmd, &cmds);
    data_free(&data);
    application_available_free(&cmds);
    return result;
}
