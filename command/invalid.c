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
    set_log_level(data->config.log_level);
    printf("%s", string_unpack(&invalid->msg));
    return invalid->code;
}

bld_handle_annotated command_handle_invalid(char* name) {
    bld_handle_annotated handle;
    (void)(name);
    handle.type = BLD_COMMAND_INVALID;
    handle.name = bld_handle_name_invalid;
    handle.convert = NULL;
    handle.execute = (bld_command_execute*) command_invalid;
    handle.free = (bld_command_free*) command_invalid_free;
    return handle;
}

void command_invalid_free(bld_command_invalid* invalid) {
    string_free(&invalid->msg);
}

