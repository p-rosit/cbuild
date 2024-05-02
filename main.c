#include "bit_core/args.h"
#include "bit_core/logging.h"
#include "bit_core/path.h"
#include "command/utils.h"
#include "command/init.h"
#include "command/remove.h"


int main(int argc, char** argv) {
    int result;
    bld_string str;
    bld_path build_root;
    bld_args args;

    args = args_new(argc, argv);
    args_advance(&args); /* Ignore how program was invoked */

    if (args_empty(&args)) {
        log_error("Incorrect usage");
        return -1;
    }

    str = args_advance(&args);
    build_root = bld_find_root();
    if (build_root.str.size == 0 && !string_eq(&str, &bld_command_init)) {
        log_error("No build file");
        path_free(&build_root);
        return -1;
    }
    log_debug("Root is: \"%s\"", path_to_string(&build_root));

    if (string_eq(&str, &bld_command_init)) {
        result = bld_init(&build_root, &args);
    } else if (string_eq(&str, &bld_command_remove)) {
        result = bld_remove(&build_root, &args);
    } else {
        log_info("bit: '%s' is not a bit command", string_unpack(&str));
        result = -1;
    }

    path_free(&build_root);
    return result;
}
