#ifndef LANGUAGE_H
#define LANGUAGE_H
#include "../dstr.h"
#include "../path.h"
#include "../array.h"
#include "../set.h"
#include "../file.h"
#include "../project_base.h"
#include "language_types.h"


bld_array language_get_available(void);
bld_language_type language_get_mapping(bld_string*);
bld_string* language_get_string(bld_language_type);

int language_get_includes(bld_language_type, bld_project_base*, bld_path*, bld_file*, bld_set*);

#endif
