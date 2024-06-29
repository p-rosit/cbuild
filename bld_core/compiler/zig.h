#ifndef COMPILER_ZIG_H
#define COMPILER_ZIG_H
#include "compiler.h"

extern bld_string bld_compiler_string_zig;

int compile_to_object_zig(bld_string*, bld_string*, bld_path*, bld_path*);
int compiler_file_is_implementation_zig(bld_string*);
int compiler_file_is_header_zig(bld_string*);
bld_language_type compiler_file_language_zig(bld_string*);

#endif
