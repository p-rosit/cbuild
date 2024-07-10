#ifndef CACHE_H
#define CACHE_H
#include "path.h"
#include "set.h"
#include "file.h"
#include "project_base.h"
#include "compiler.h"

bld_cache_handle cache_handle_new(bld_project_base*, bld_path*);
void cache_handle_free(bld_cache_handle*);
int cache_object_get(bld_cache_handle*, bld_file*, bld_compiler*, bld_path*);
int cache_includes_get(bld_cache_handle*, bld_file*, bld_compiler*, bld_set*);
int cache_symbols_get(bld_cache_handle*, bld_file*, bld_compiler*, bld_set*, bld_set*);

#endif
