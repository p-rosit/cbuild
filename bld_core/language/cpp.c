#include "../logging.h"
#include "utils.h"
#include "c.h"
#include "cpp.h"

bld_string bld_language_string_cpp = STRING_COMPILE_TIME_PACK("cpp");

int language_get_includes_cpp(bld_path* path, bld_file* file, bld_set* files) {
    return language_get_includes_c(path, file, files);
}

int language_get_symbols_cpp(bld_project_base* base, bld_path* path, bld_file* file) {
    return language_get_symbols_c(base, path, file);
}
