#include "logging.h"
#include "os.h"
#include "cache.h"

typedef struct bld_cache_entry {
    int referenced;
    bld_string dir_name;
    int builds_since_access;
    bld_path path;
    bld_time mtime;
} bld_cache_entry;

typedef struct bld_cache_file {
    bld_string name;
    bld_linker linker;
    bld_compiler_or_flags compiler;
    bld_set includes;
    bld_set undefined_symbols;
    bld_set defined_symbols;
} bld_cache_file;

void cache_handle_purge(bld_cache_handle*);
int cache_handle_parse(bld_cache_handle*);

bld_cache_handle cache_handle_new(bld_project_base* base, bld_path* root) {
    int error;
    bld_cache_handle cache;

    cache.base = base;
    cache.root = *root;
    cache.files = set_new(sizeof(bld_cache_entry));
    cache.loaded_files = set_new(sizeof(bld_cache_file));

    if (!os_dir_exists(path_to_string(root))) {
        os_dir_make(path_to_string(root));
    }

    error = cache_handle_parse(&cache);
    if (error) {
        log_error(LOG_FATAL_PREFIX "cache handle info could not be parsed, clearing cache");
    }

    cache_handle_purge(&cache);
    return cache;
}

void cache_handle_purge(bld_cache_handle* cache) {
}

int cache_handle_parse(bld_cache_handle* cache) {
    return 0;
}
