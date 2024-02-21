#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include "path.h"
#include "file.h"

#define BLD_CACHE_NAME ("cache.json")

typedef struct bld_cache {
    bld_path path;
    bld_files files;
} bld_cache;

bld_cache   new_cache(bld_path*);
void        free_cache(bld_cache*);
void        serialize_cache(bld_cache*);
void        parse_cache(bld_cache*, bld_path*);

#endif
