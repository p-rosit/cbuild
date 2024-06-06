#ifndef COMPILER_COMPILER_H
#define COMPILER_COMPILER_H
#include "../dstr.h"
#include "../path.h"
#include "../array.h"
#include "../set.h"

typedef enum bld_compiler_handle_type {
    BLD_COMPILER_GCC,
    BLD_COMPILER_CLANG,
    BLD_COMPILER_HANDLE_AMOUNT
} bld_compiler_handle_type;

bld_array compiler_get_available(void);
bld_compiler_handle_type compiler_get_mapping(bld_string*);
bld_string* compiler_get_string(bld_compiler_handle_type);
bld_string compiler_get_file_extension(bld_string*);
int compiler_file_is_implementation(bld_set*, bld_string*);
int compiler_file_is_header(bld_set*, bld_string*);

#endif
