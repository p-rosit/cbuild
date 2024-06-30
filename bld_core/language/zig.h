#ifndef LANGUAGE_ZIG_H
#define LANGUAGE_ZIG_H
#include "language.h"

extern bld_string bld_language_string_zig;

int language_get_includes_zig(bld_path*, bld_file*, bld_set*);
int language_get_symbols_zig(bld_project_base*, bld_path*, bld_file*);

#endif
