#include "c.h"
#include "zig.h"

bld_string bld_language_string_zig = STRING_COMPILE_TIME_PACK("zig");

int language_get_includes_zig(bld_path* path, bld_file* file, bld_set* files) {
    (void)(path);
    (void)(file);
    (void)(files);
    return 0;
}

int language_get_symbols_zig(bld_project_base* base, bld_path* path, bld_file* file) {
    return language_get_symbols_c(base, path, file);
}
