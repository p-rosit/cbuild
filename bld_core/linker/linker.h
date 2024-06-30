#ifndef LINKER_LINKER_H
#define LINKER_LINKER_H
#include "../dstr.h"
#include "../path.h"
#include "../array.h"

typedef enum bld_linker_type {
    BLD_LINKER_GCC,
    BLD_LINKER_CLANG,
    BLD_LINKER_ZIG,
    BLD_LINKER_AMOUNT
} bld_linker_type;

bld_array linker_get_available(void);
bld_linker_type linker_get_mapping(bld_string*);
bld_string* linker_get_string(bld_linker_type);

int linker_executable_make(bld_linker_type, bld_string*, bld_path*, bld_array*, bld_array*, bld_path*);

#endif
