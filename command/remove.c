#include "../bit_core/logging.h"
#include "invalid.h"
#include "remove.h"

const bld_string bld_command_string_remove = STRING_COMPILE_TIME_PACK("remove");

int command_remove(bld_command_remove* remove, bld_data* data) {
    log_debug("Removing target: \"%s\"", string_unpack(&remove->target));
    (void)(data);
    return -1;
}

int command_remove_parse(bld_args* args, bld_command_remove* remove, bld_command_invalid* invalid) {
    bld_string error_msg;
    bld_string target;

    if (args_empty(args)) {
        error_msg = string_pack("missing target");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    target = args_advance(args);

    if (!args_empty(args)) {
        error_msg = string_pack("too many args");
        invalid->code = -1;
        invalid->msg = string_copy(&error_msg);
        return -1;
    }

    remove->target = string_copy(&target);
    return 0;
}

void command_remove_free(bld_command_remove* remove) {
    string_free(&remove->target);
}
