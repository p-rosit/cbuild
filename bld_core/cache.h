#ifndef CACHE_H
#define CACHE_H
#include "path.h"
#include "set.h"
#include "file.h"
#include "compiler.h"

typedef struct bld_cache_handle {
    bld_path root;
    bld_set files;
    bld_set loaded_files;
} bld_cache_handle;

bld_cache_handle cache_new(bld_path*);
int cache_object_get(bld_file*, bld_compiler*);

#endif
