#include <stdio.h>
#include "build.h"
#include "cache.h"

bld_cache new_cache(bld_path* path) {
    return (bld_cache) {
        .path = *path,
        .files = new_files(),
    };
}

void free_cache(bld_cache* cache) {
    if (cache == NULL) {return;}
    free_path(&cache->path);
    free_files(&cache->files);
}

void serialize_cache(bld_cache* cache) {
    log_warn("serialize_cache: Serialization not implemented.");
}

void parse_cache(bld_cache* cache) {
    log_warn("parse_cache: Parsing not implemented");
}
