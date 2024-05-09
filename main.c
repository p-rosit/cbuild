#include "bld_core/args.h"
#include "bld_core/logging.h"
#include "bld_core/path.h"
#include "command/command.h"

int main(int argc, char** argv) {
    int result;
    bld_args args;
    bld_data data;
    bld_command cmd;

    args = args_new(argc, argv);
    args_advance(&args); /* Ignore how program was invoked */

    data = data_extract();
    cmd = command_parse(&args, &data);
    result = command_execute(&cmd, &data);

    command_free(&cmd);
    data_free(&data);
    return result;
}
