#ifndef LANGUAGE_CPP_H
#define LANGUAGE_CPP_H
#include "language.h"

extern bld_string bld_language_string_cpp;

int language_get_includes_cpp(bld_path*, bld_file*, bld_set*);
int language_get_symbols_cpp(bld_project_base*, bld_path*, bld_file*);

#endif
