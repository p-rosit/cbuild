#ifndef LANGUAGE_C_H
#define LANGUAGE_C_H
#include "language.h"

extern bld_string bld_language_string_c;

int language_get_includes_c(bld_path*, bld_file*, bld_set*);
int language_get_symbols_c(bld_project_base*, bld_path*, bld_file*);

#endif
