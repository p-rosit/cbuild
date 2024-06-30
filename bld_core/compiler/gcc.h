#ifndef COMPILER_GCC_H
#define COMPILER_GCC_H
#include "compiler.h"

extern bld_string bld_compiler_string_gcc;
extern bld_string bld_compiler_string_gpp;

int compile_to_object_gcc(bld_string*, bld_string*, bld_path*, bld_path*);
int compiler_file_is_implementation_gcc(bld_string*);
int compiler_file_is_header_gcc(bld_string*);
bld_language_type compiler_file_language_gcc(bld_string*);

#endif
