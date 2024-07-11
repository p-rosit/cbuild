#ifndef COMPILER_CLANG_H
#define COMPILER_CLANG_H
#include "compiler.h"

extern bld_string bld_compiler_string_clang;

int compile_to_object_clang(bld_compiler*, bld_path*, bld_path*);
int compiler_file_is_implementation_clang(bld_string*);
int compiler_file_is_header_clang(bld_string*);
bld_language_type compiler_file_language_clang(bld_string*);

#endif
