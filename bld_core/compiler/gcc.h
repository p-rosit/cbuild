#ifndef COMPILER_GCC_H
#define COMPILER_GCC_H
#include "compiler.h"

extern bld_string bld_compiler_string_gcc;
extern bld_string bld_compiler_string_gpp;

int compile_to_object_gcc(bld_string*, bld_path*, bld_string*);
int compiler_file_is_implementation_gcc(bld_string*);
int compiler_file_is_header_gcc(bld_string*);

#endif