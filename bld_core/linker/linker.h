#ifndef LINKER_LINKER_H
#define LINKER_LINKER_H
#include "../dstr.h"
#include "../path.h"
#include "../array.h"
#include "../linker.h"

bld_array linker_get_available(void);
bld_linker_type linker_get_mapping(bld_string*);
bld_string* linker_get_string(bld_linker_type);

int linker_executable_make(bld_linker*, bld_path*, bld_array*, bld_array*, bld_path*);

#endif
