#include "logging.h"
#include "args.h"

bld_args args_new(int argc, char** argv) {
    bld_args args;
    args.argc = argc;
    args.argv = argv;
    return args;
}

int args_empty(bld_args* args) {
    return args->argc <= 0;
}

bld_string args_advance(bld_args* args) {
    if (args->argc <= 0) {
        log_fatal("args_advance: attempting to advance exhausted argument list");
    }
    args->argc--;
    return string_pack(*args->argv++);
}

bld_string args_next(bld_args* args) {
    return string_pack(args->argv[0]);
}
