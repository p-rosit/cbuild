#include "../bit_core/logging.h"
#include "remove.h"

const bld_string bld_command_remove = STRING_COMPILE_TIME_PACK("remove");

int bld_remove(bld_path* root, bld_args* args) {
    log_debug("Removing target");
    (void)(root);
    (void)(args);
    return -1;
}
