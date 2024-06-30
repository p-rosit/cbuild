#include "clang.h"
#include "gcc.h"

bld_string bld_linker_string_clang = STRING_COMPILE_TIME_PACK("clang");

int linker_executable_make_clang(bld_string* linker, bld_path* root, bld_array* files, bld_array* flags, bld_path* name) {
    return linker_executable_make_gcc(linker, root, files, flags, name);
}
