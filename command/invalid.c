#include "../bld_core/logging.h"
#include "invalid.h"

bld_command_invalid command_invalid_new(int code, bld_string* msg) {
    bld_command_invalid invalid;
    invalid.code = code;
    invalid.msg = *msg;
    return invalid;
}

int command_invalid(bld_command_invalid* invalid, bld_data* data) {
    (void)(data);
    log_error("Error: %s", string_unpack(&invalid->msg));
    return invalid->code;
}

void command_invalid_free(bld_command_invalid* invalid) {
    string_free(&invalid->msg);
}

