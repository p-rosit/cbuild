#ifndef COMPILER_COMPILER_H
#define COMPILER_COMPILER_H
#include "../dstr.h"
#include "../path.h"
#include "../array.h"
#include "../set.h"
#include "../compiler.h"
#include "../language/language_types.h"

bld_array compiler_get_available(void);
bld_compiler_type compiler_get_mapping(bld_string*);
bld_string* compiler_get_string(bld_compiler_type);
bld_string compiler_get_file_extension(bld_string*);
int compile_to_object(bld_compiler*, bld_path*, bld_path*);
int compiler_file_is_implementation(bld_set*, bld_string*);
int compiler_file_is_header(bld_set*, bld_string*);
bld_language_type compiler_file_language(bld_compiler_type, bld_string*);

#endif
