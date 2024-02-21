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

void parse_cache(bld_cache* cache, bld_path* root) {
    bld_path path = copy_path(root);
    FILE* f;

    append_path(&path, &cache->path);
    append_dir(&path, BLD_CACHE_NAME);
    f = fopen(path_to_string(&path), "r");

    log_warn("parse_cache: not implemented");
    
    fclose(f);
    free_path(&path);
}
