#include <limits.h>
#include "logging.h"
#include "json.h"
#include "os.h"
#include "cache.h"
#include "language/language.h"

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
    int includes_parsed;
    bld_set includes;
    int symbols_parsed;
    bld_set undefined_symbols;
    bld_set defined_symbols;
} bld_cache_file;

void cache_entry_free(bld_cache_entry*);
void cache_file_free(bld_cache_file*);

void cache_handle_purge(bld_cache_handle*);
int cache_handle_parse(bld_cache_handle*);
int parse_cache_handle(FILE*, bld_cache_handle*);
int parse_cache_handle_files(FILE*, bld_cache_handle*);
int parse_cache_handle_file(FILE*, bld_array*);
int parse_cache_handle_file_name(FILE*, bld_cache_entry*);
int parse_cache_handle_file_builds(FILE*, bld_cache_entry*);
int parse_cache_handle_file_path(FILE*, bld_cache_entry*);
int parse_cache_handle_file_mtime(FILE*, bld_cache_entry*);

void serialize_cache_handle(bld_cache_handle*);
void serialize_cache_handle_loaded_files(bld_cache_handle*);

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

void cache_handle_free(bld_cache_handle* cache) {
    bld_iter iter;
    bld_cache_entry* entry;

    if (!cache->loaded) {
        return;
    }

    path_free(&cache->root);

    iter = iter_set(&cache->files);
    while (iter_next(&iter, (void**) &entry)) {
        cache_entry_free(entry);
    }
    set_free(&cache->files);

    set_free(&cache->loaded_files);
}

void cache_handle_purge(bld_cache_handle* cache) {
    (void)(cache);
    log_error(LOG_FATAL_PREFIX "don't forget to purge cache");
}

void cache_entry_free(bld_cache_entry* entry) {
    string_free(&entry->dir_name);
    path_free(&entry->path);
}

int cache_object_get(bld_cache_handle* cache, bld_compiler* compiler, bld_file* file, bld_path* path, uintmax_t main_id) {
    int error;
    int cached;

    error = 0;
    cached = 0;
    if (set_has(&cache->files, file->identifier.id)) {
        bld_set* includes;

        if (set_has(&cache->loaded_files, file->identifier.id)) {
            error = 0;
            // TODO: get path from cache
        } else {
            // TODO: load cache entry
        }
    }

    if (!cached) {
        bld_path path;

        if (!cache->base->rebuilding || file->identifier.id != main_id) {
            path = path_copy(&cache->base->root);
        } else {
            path = path_copy(&cache->base->build_of->root);
        }

        error = compile_to_object(compiler, &path, &object_path);

        path_free(&path);
    }

    return error;
}

int cache_includes_get(bld_cache_handle* cache, bld_compiler* compiler, bld_file* file, bld_set* files, uintmax_t main_id) {
    int error;
    int cached;

    error = 0;
    cached = 0;
    if (set_has(&cache->files, file->identifier.id)) {
        bld_set* includes;
        includes = file_includes_get(file);
        if (includes == NULL) {
            log_fatal(LOG_FATAL_PREFIX "file does not have includes");
        }

        if (set_has(&cache->loaded_files, file->identifier.id)) {
            error = 0;
            // TODO: copy cached includes
        } else {
            // TODO: load cache entry
        }
    }

    if (!cached) {
        bld_path path;

        if (!cache->base->rebuilding || file->identifier.id != main_id) {
            path = path_copy(&cache->base->root);
        } else {
            path = path_copy(&cache->base->build_of->root);
        }

        (void)(compiler);  /* TODO: use compiler to determine includes? */
        error = language_get_includes(file->language, &path, file, files);

        path_free(&path);
    }

    return error;
}

int cache_symbols_get(bld_cache_handle* cache, bld_compiler* compiler, bld_file* file, uintmax_t main_id) {
    int error;
    int cached;

    error = 0;
    cached = 0;
    if (set_has(&cache->files, file->identifier.id)) {
        bld_set* defined_symbols;
        bld_set* undefined_symbols;

        defined_symbols = file_defined_get(file);
        undefined_symbols = file_undefined_get(file);
        if (defined_symbols == NULL && undefined_symbols == NULL) {
            log_fatal(LOG_FATAL_PREFIX "file does not have symbols");
        }

        if (set_has(&cache->loaded_files, file->identifier.id)) {
            error = 0;
            // TODO: copy cached symbols
        } else {
            // TODO: load cache entry
        }
    }

    if (!cached) {
        bld_path path;

        if (!cache->base->rebuilding || file->identifier.id != main_id) {
            path = path_copy(&cache->base->root);
        } else {
            path = path_copy(&cache->base->build_of->root);
        }

        (void)(compiler);  /* TODO: use compiler to determine symb? */
        error = language_get_symbols(file->language, cache->base, &path, file);

        path_free(&path);
    }

    return error;
}

int cache_handle_parse(bld_cache_handle* cache) {
    FILE* file;
    int error;
    bld_path path;

    path = path_copy(&cache->base->root);
    path_append_path(&path, &cache->root);
    path_append_string(&path, "cache.json");
    file = fopen(path_to_string(&path), "r");

    error = 0;
    if (file == NULL) {
        log_debug("No cache file found.");
    } else {
        error = parse_cache_handle(file, cache);
        fclose(file);

        if (error) {
            log_warn("Could not parse cache, ignoring.");
        } else {
            log_dinfo("Loaded cache");
        }
    }

    path_free(&path);
    return error;
}

int parse_cache_handle(FILE* file, bld_cache_handle* cache) {
    int size = 1;
    int parsed[1];
    char *keys[1] = {"files"};
    bld_parse_func funcs[1] = {
        (bld_parse_func) parse_cache_handle_files,
    };

    json_parse_map(file, cache, size, parsed, keys, funcs);
    if (!parsed[0]) {
        return -1;
    }

    return 0;
}

int parse_cache_handle_files(FILE* file, bld_cache_handle* cache) {
    int amount_parsed;
    bld_array files;

    files = array_new(sizeof(bld_cache_entry));
    amount_parsed = json_parse_array(file, &files, (bld_parse_func) parse_cache_handle_file);
    if (amount_parsed < 0) {
        log_fatal(LOG_FATAL_PREFIX "free correctly");
        return -1;
    }

    {
        bld_iter iter;
        bld_cache_entry* entry;

        iter = iter_array(&files);
        while (iter_next(&iter, (void**) &entry)) {
            bld_path path;
            bld_file_id id;

            path = path_copy(&cache->base->root);
            path_append_path(&path, &entry->path);

            id = file_get_id(&path);
            if (id == BLD_INVALID_IDENITIFIER) {
                cache_entry_free(entry);
            } else {
                set_add(&cache->files, id, entry);
            }

            path_free(&path);
        }

        array_free(&files);
    }

    return 0;
}

int parse_cache_handle_file(FILE* file, bld_array* entries) {
    int amount_parsed;
    int size = 4;
    int parsed[4];
    char *keys[4] = {"name", "builds_since_access", "path", "mtime"};
    bld_parse_func funcs[4] = {
        (bld_parse_func) parse_cache_handle_file_name,
        (bld_parse_func) parse_cache_handle_file_builds,
        (bld_parse_func) parse_cache_handle_file_path,
        (bld_parse_func) parse_cache_handle_file_mtime,
    };
    bld_cache_entry entry;

    amount_parsed = json_parse_map(file, &entry, size, parsed, keys, funcs);
    if (amount_parsed != size) {
        log_fatal(LOG_FATAL_PREFIX "free correctly");
        return -1;
    }

    array_push(entries, &entry);
    return 0;
}

int parse_cache_handle_file_name(FILE* file, bld_cache_entry* entry) {
    int error;
    bld_string str;

    error = string_parse(file, &str);
    if (error) {
        return -1;
    }

    entry->dir_name = str;
    return 0;
}

int parse_cache_handle_file_builds(FILE* file, bld_cache_entry* entry) {
    int error;
    uintmax_t num;

    error = parse_uintmax(file, &num);
    if (error) {
        return -1;
    }

    if (num > INT_MAX) {
        return -1;
    }
    entry->builds_since_access = (int) num;
    return 0;
}

int parse_cache_handle_file_path(FILE* file, bld_cache_entry* entry) {
    int error;
    bld_string str;

    error = string_parse(file, &str);
    if (error) {
        return -1;
    }

    entry->path = path_from_string(string_unpack(&str));
    string_free(&str);
    return 0;
}

int parse_cache_handle_file_mtime(FILE* file, bld_cache_entry* entry) {
    int error;
    uintmax_t num;

    error = parse_uintmax(file, &num);
    if (error) {
        return -1;
    }

    entry->mtime = num;
    return 0;
}
