#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include "os.h"
#include "logging.h"
#include "path.h"
#include "project.h"
#include "json.h"

typedef struct bld_parsing_file {
    bld_project_cache* cache;
    bld_file file;
    uintmax_t parent;
    int is_rebuild_main;
} bld_parsing_file;

typedef enum bld_file_fields {
    BLD_PARSE_TYPE = 0,
    BLD_PARSE_MTIME = 1,
    BLD_PARSE_HASH = 2,
    BLD_PARSE_NAME = 3,
    BLD_PARSE_COMPILER = 4,
    BLD_PARSE_COMPILER_FLAGS = 5,
    BLD_PARSE_LINKER_FLAGS = 6,
    BLD_PARSE_INCLUDES = 7,
    BLD_PARSE_DEFINED = 8,
    BLD_PARSE_UNDEFINED = 9,
    BLD_PARSE_FILES = 10,
    BLD_TOTAL_FIELDS = 11
} bld_file_fields;

void ensure_directory_exists(bld_path*);
int parse_cache(bld_project_cache*, bld_path*);
int parse_project_linker(FILE*, bld_project_cache*);
int parse_project_rebuild_main(FILE*, bld_project_cache*);

int parse_project_files(FILE*, bld_project_cache*);
int parse_file(FILE*, bld_parsing_file*);
int parse_file_type(FILE*, bld_parsing_file*);
int parse_file_mtime(FILE*, bld_parsing_file*);
int parse_file_hash(FILE*, bld_parsing_file*);
int parse_file_name(FILE*, bld_parsing_file*);
int parse_file_compiler(FILE*, bld_parsing_file*);
int parse_file_compiler_flags(FILE*, bld_parsing_file*);
int parse_file_linker_flags(FILE*, bld_parsing_file*);
int parse_file_defined_symbols(FILE*, bld_parsing_file*);
int parse_file_undefined_symbols(FILE*, bld_parsing_file*);
int parse_file_function(FILE*, bld_set*);
int parse_file_includes(FILE*, bld_parsing_file*);
int parse_file_include(FILE*, bld_array*);
int parse_file_sub_files(FILE*, bld_parsing_file*);
int parse_file_sub_file(FILE*, bld_parsing_file*);

void ensure_directory_exists(bld_path* directory_path) {
    errno = 0;
    os_dir_make(path_to_string(directory_path));
    switch (errno) {
        case 0: {
            log_debug("Creating cache directory.");
        } break;
        case EACCES: {
            log_fatal("No access to create cache directory.");
        } break;
        case EEXIST: {
            log_debug("Found cache directory.");
        } break;
        case ENAMETOOLONG: {
            log_fatal("Name of cache directory too long.");
        } break;
        case EROFS: {
            log_fatal("Cache directory cannot be created in a read-only file system.");
        } break;
        default: {perror("Unreachable error?"); log_fatal("project_load_cache: unreachable error???");}
    }
}

void project_load_cache(bld_forward_project* fproject, char* cache_path) {
    FILE* file;
    bld_path path;

    path = path_copy(&fproject->base.root);
    path_append_string(&path, cache_path);
    ensure_directory_exists(&path);

    path_append_string(&path, BLD_CACHE_NAME);
    file = fopen(path_to_string(&path), "r");

    fproject->base.cache.loaded = 1;
    fproject->base.cache.root = path_from_string(cache_path);
    fproject->base.cache.files = set_new(sizeof(bld_file));

    if (file == NULL) {
        log_debug("No cache file found.");
    } else {
        int error;

        fclose(file);
        log_debug("Found cache file, attempting to parse.");
        error = parse_cache(&fproject->base.cache, &fproject->base.root);

        if (error) {
            log_warn("Could not parse cache, ignoring.");
        } else {
            log_info("Loaded cache.");
            fproject->base.cache.set = 1;
        }
    }

    path_free(&path);
}

int parse_cache(bld_project_cache* cache, bld_path* root) {
    int size = 3;
    int parsed[3];
    char *keys[3] = {"linker", "files", "rebuild_main"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_project_linker,
        (bld_parse_func) parse_project_files,
        (bld_parse_func) parse_project_rebuild_main,
    };
    bld_path path = path_copy(root);
    FILE* f;

    path_append_path(&path, &cache->root);
    path_append_string(&path, BLD_CACHE_NAME);
    f = fopen(path_to_string(&path), "r");

    cache->root_file = BLD_INVALID_IDENITIFIER;
    json_parse_map(f, cache, size, parsed, keys, funcs);

    if (!parsed[0] || !parsed[1] || (cache->base->rebuilding && !parsed[2])) {
        fclose(f);
        path_free(&path);

        if (parsed[0]) {
            linker_free(&cache->linker);
        }

        if (parsed[1]) {
            bld_iter iter;
            bld_file* file;

            iter = iter_set(&cache->files);
            while (iter_next(&iter, (void**) &file)) {
                file_free(file);
            }
            set_free(&cache->files);
        }

        if (parsed[2]) {
            log_fatal(LOG_FATAL_PREFIX "free correctly");
        }

        return -1;
    }

    fclose(f);
    path_free(&path);
    return 0;
}

int parse_project_linker(FILE* file, bld_project_cache* cache) {
    int error;

    error = parse_linker(file, &cache->linker);
    if (error) {log_warn("Could not parse linker for project.");}

    return error;
}

int parse_project_files(FILE* file, bld_project_cache* cache) {
    int error;
    bld_parsing_file f;

    f.cache = cache;
    f.parent = BLD_INVALID_IDENITIFIER;
    f.is_rebuild_main = 0;

    error = parse_file(file, &f);
    if (error) {
        bld_iter iter;
        bld_file* file;

        iter = iter_set(&cache->files);
        while (iter_next(&iter, (void**) &file)) {
            file_free(file);
        }
        set_free(&cache->files);

        log_warn("Could not parse file tree");
        return -1;
    }

    cache->root_file = f.file.identifier.id;
    return 0;
}

int parse_file(FILE* file, bld_parsing_file* f) {
    int amount_parsed;
    int parsed[BLD_TOTAL_FIELDS];
    char *keys[BLD_TOTAL_FIELDS] = {
        "type",
        "mtime",
        "hash",
        "name",
        "compiler",
        "compiler_flags",
        "linker_flags",
        "includes",
        "defined_symbols",
        "undefined_symbols",
        "files",
    };
    bld_parse_func funcs[BLD_TOTAL_FIELDS] = {
        (bld_parse_func) parse_file_type,
        (bld_parse_func) parse_file_mtime,
        (bld_parse_func) parse_file_hash,
        (bld_parse_func) parse_file_name,
        (bld_parse_func) parse_file_compiler,
        (bld_parse_func) parse_file_compiler_flags,
        (bld_parse_func) parse_file_linker_flags,
        (bld_parse_func) parse_file_includes,
        (bld_parse_func) parse_file_defined_symbols,
        (bld_parse_func) parse_file_undefined_symbols,
        (bld_parse_func) parse_file_sub_files,
    };

    f->file.type = BLD_FILE_INVALID;
    f->file.path = path_new();
    f->file.build_info.compiler_set = 0;
    f->file.build_info.linker_set = 0;

    amount_parsed = json_parse_map(file, f, BLD_TOTAL_FIELDS, parsed, keys, funcs);

    if (amount_parsed < 0) {goto parse_failed;}
    if (!parsed[BLD_PARSE_TYPE]) {goto parse_failed;} /* Fail if file type was not parsed */

    switch (f->file.type) {
        case (BLD_FILE_DIRECTORY): {
            if (
                !parsed[BLD_PARSE_NAME]
                || !parsed[BLD_PARSE_FILES]
            ) {
                log_warn("Directory requires the following fields: [\"%s\", \"%s\"]", keys[BLD_PARSE_NAME], keys[BLD_PARSE_FILES]);
                goto parse_failed;
            }
            if (
                parsed[BLD_PARSE_MTIME]
                || parsed[BLD_PARSE_HASH]
                || parsed[BLD_PARSE_INCLUDES]
                || parsed[BLD_PARSE_DEFINED]
                || parsed[BLD_PARSE_UNDEFINED]
            ) {
                log_warn("Directory cannot have any of the following fields: [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_MTIME], keys[BLD_PARSE_HASH], keys[BLD_PARSE_INCLUDES], keys[BLD_PARSE_DEFINED], keys[BLD_PARSE_UNDEFINED]);
                goto parse_failed;
            }
        } break;
        case (BLD_FILE_IMPLEMENTATION): {
            if (
                !parsed[BLD_PARSE_MTIME]
                || !parsed[BLD_PARSE_HASH]
                || !parsed[BLD_PARSE_NAME]
                || !parsed[BLD_PARSE_INCLUDES]
                || !parsed[BLD_PARSE_DEFINED]
                || !parsed[BLD_PARSE_UNDEFINED]
            ) {
                log_warn("Implementation file requires the following fields: [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_MTIME], keys[BLD_PARSE_HASH], keys[BLD_PARSE_NAME], keys[BLD_PARSE_INCLUDES], keys[BLD_PARSE_DEFINED], keys[BLD_PARSE_UNDEFINED]);
                goto parse_failed;
            }
            if (
                parsed[BLD_PARSE_FILES]
            ) {
                log_warn("Implementation file cannot have any of the following fields: [\"%s\"]", keys[BLD_PARSE_FILES]);
                goto parse_failed;
            }
        } break;
        case (BLD_FILE_TEST): {
            if (
                !parsed[BLD_PARSE_MTIME]
                || !parsed[BLD_PARSE_HASH]
                || !parsed[BLD_PARSE_NAME]
                || !parsed[BLD_PARSE_INCLUDES]
                || !parsed[BLD_PARSE_UNDEFINED]
            ) {
                log_warn("Test file requires the following fields: [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_MTIME], keys[BLD_PARSE_HASH], keys[BLD_PARSE_NAME], keys[BLD_PARSE_INCLUDES], keys[BLD_PARSE_UNDEFINED]);
                goto parse_failed;
            }
            if (
                parsed[BLD_PARSE_DEFINED]
                || parsed[BLD_PARSE_FILES]
            ) {
                log_warn("Test file cannot have any of the following fields: [\"%s\", \"%s\"]", keys[BLD_PARSE_DEFINED], keys[BLD_PARSE_FILES]);
                goto parse_failed;
            }
        } break;
        case (BLD_FILE_INTERFACE): {
            if (
                !parsed[BLD_PARSE_MTIME]
                || !parsed[BLD_PARSE_HASH]
                || !parsed[BLD_PARSE_NAME]
                || !parsed[BLD_PARSE_INCLUDES]
            ) {
                log_warn("Header file requires the following fields: [\"%s\", \"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_MTIME], keys[BLD_PARSE_HASH], keys[BLD_PARSE_NAME], keys[BLD_PARSE_INCLUDES]);
                goto parse_failed;
            }
            if (
                parsed[BLD_PARSE_DEFINED]
                || parsed[BLD_PARSE_UNDEFINED]
                || parsed[BLD_PARSE_FILES]
            ) {
                log_warn("Test file cannot have any of the following fields: [\"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_DEFINED], keys[BLD_PARSE_UNDEFINED], keys[BLD_PARSE_FILES]);
                goto parse_failed;
            }
        } break;
        default: {
            log_warn("parse_file: internal error");
            goto parse_failed;
        }
    }

    set_add(&f->cache->files, f->file.identifier.id, &f->file);

    return 0;

    parse_failed:
    path_free(&f->file.path);
    if (parsed[BLD_PARSE_NAME]) {
        string_free(&f->file.name);
    }

    if (parsed[BLD_PARSE_COMPILER]) {
        compiler_free(&f->file.build_info.compiler.as.compiler);
    }

    if (parsed[BLD_PARSE_COMPILER_FLAGS]) {
        compiler_flags_free(&f->file.build_info.compiler.as.flags);
    }

    if (parsed[BLD_PARSE_LINKER_FLAGS]) {
        linker_flags_free(&f->file.build_info.linker_flags);
    }

    if (parsed[BLD_PARSE_INCLUDES]) {
        bld_set* includes;
        bld_iter iter;
        bld_path* path;

        includes = file_includes_get(&f->file);
        if (includes == NULL) {goto includes_freed;}

        iter = iter_set(includes);
        while (iter_next(&iter, (void**) &path)) {
            path_free(path);
        }
        set_free(includes);
    }
    includes_freed:

    if (parsed[BLD_PARSE_DEFINED]) {
        bld_set* defined;
        bld_iter iter;
        bld_string* str;

        defined = file_defined_get(&f->file);
        if (defined == NULL) {
            goto defined_freed;
        }

        iter = iter_set(defined);
        while (iter_next(&iter, (void**) &str)) {
            string_free(str);
        }
        set_free(defined);
    }
    defined_freed:

    if (parsed[BLD_PARSE_UNDEFINED]) {
        bld_set* undefined;
        bld_iter iter;
        bld_string* str;

        undefined = file_undefined_get(&f->file);
        if (undefined == NULL) {
            goto undefined_freed;
        }

        iter = iter_set(undefined);
        while (iter_next(&iter, (void**) &str)) {
            string_free(str);
        }
        set_free(undefined);
    }
    undefined_freed:

    return -1;
}

int parse_file_type(FILE* file, bld_parsing_file* f) {
    bld_string str;
    char* temp;
    int error;

    error = string_parse(file, &str);
    if (error) {
        log_warn("Could not parse compiler type");
        return -1;
    }
    
    temp = string_unpack(&str);
    if (strcmp(temp, "directory") == 0) {
        f->file.type = BLD_FILE_DIRECTORY;
    } else if (strcmp(temp, "implementation") == 0) {
        f->file.type = BLD_FILE_IMPLEMENTATION;
        f->file.info.impl.undefined_symbols = set_new(sizeof(bld_string));
        f->file.info.impl.defined_symbols = set_new(sizeof(bld_string));
    } else if (strcmp(temp, "interface") == 0) {
        f->file.type = BLD_FILE_INTERFACE;
    } else if (strcmp(temp, "test") == 0) {
        f->file.type = BLD_FILE_TEST;
        f->file.info.test.undefined_symbols = set_new(sizeof(bld_string));
    } else {
        log_warn("Not a valid file type: \"%s\"", temp);
        error = -1;
    }

    string_free(&str);
    return error;
}

int parse_file_mtime(FILE* file, bld_parsing_file* f) {
    uintmax_t num;
    int error;

    error = parse_uintmax(file, &num);
    if (error) {
        log_warn("Could not parse file mtime");
        return -1;
    }

    f->file.identifier.time = num;
    return 0;
}

int parse_file_hash(FILE* file, bld_parsing_file* f) {
    uintmax_t num;
    int error;

    error = parse_uintmax(file, &num);
    if (error) {
        log_warn("Could not parse file id");
        return -1;
    }

    f->file.identifier.hash = num;
    return 0;
}

int parse_file_name(FILE* file, bld_parsing_file* f) {
    bld_string str;
    int error;

    error = string_parse(file, &str);
    if (error) {
        log_warn("Could not parse file name");
        return -1;
    }
    
    f->file.name = str;
    return error;
}

int parse_file_compiler(FILE* file, bld_parsing_file* f) {
    int error;
    bld_compiler compiler;

    if (f->file.build_info.compiler_set) {
        log_warn("parse_file_compiler: could not parse cache, \"%s\" has multiple mutually exclusive fields set (compiler and compiler_flags)", string_unpack(&f->file.name));
        return -1;
    }

    error = parse_compiler(file, &compiler);
    if (error) {
        log_warn("Could not parse file compiler");
    }

    f->file.build_info.compiler_set = 1;
    f->file.build_info.compiler.type = BLD_COMPILER;
    f->file.build_info.compiler.as.compiler = compiler;
    return error;
}

int parse_file_compiler_flags(FILE* file, bld_parsing_file* f) {
    int error;
    bld_compiler_flags flags;

    if (f->file.build_info.compiler_set) {
        log_warn("parse_file_compiler_flags: could not parse cache, \"%s\" has multiple mutually exclusive fields set (compiler and compiler_flags)", string_unpack(&f->file.name));
        return -1;
    }

    error = parse_compiler_flags(file, &flags);
    if (error) {
        log_warn("Could not parse file compiler flags");
    }

    f->file.build_info.compiler_set = 1;
    f->file.build_info.compiler.type = BLD_COMPILER_FLAGS;
    f->file.build_info.compiler.as.flags = flags;
    return error;
}

int parse_file_linker_flags(FILE* file, bld_parsing_file* f) {
    int error;

    f->file.build_info.linker_flags = linker_flags_new();
    error = parse_linker_flags(file, &f->file.build_info.linker_flags);
    if (error) {
        log_warn("Could not parse linker flags");
    }

    f->file.build_info.linker_set = 1;
    return error;
}

int parse_file_includes(FILE* file, bld_parsing_file* f) {
    int amount_parsed;
    bld_set *includes;
    bld_array includes_array;

    includes_array = array_new(sizeof(bld_path));
    amount_parsed = json_parse_array(file, &includes_array, (bld_parse_func) parse_file_include);
    if (amount_parsed < 0) {
        array_free(&includes_array);
        log_warn("Could not parse file includes");
        return -1;
    }

    includes = file_includes_get(&f->file);
    if (includes == NULL) {
        log_fatal(LOG_FATAL_PREFIX "attempting to set includes of file which does not have includes");
    }

    {
        bld_set resolved_includes;
        bld_path root;
        bld_path* include_path;
        bld_iter iter;

        if (f->cache->base->rebuilding) {
            root = path_copy(&f->cache->base->build_of->root);
            log_error("Checking \"%s\"", path_to_string(&root));
        } else {
            root = path_new();
        }
        path_append_path(&root, &f->cache->base->root);
        log_error("Adding \"%s\"", path_to_string(&f->cache->base->root));

        resolved_includes = set_new(sizeof(bld_path));
        iter = iter_array(&includes_array);
        while (iter_next(&iter, (void**) &include_path)) {
            bld_path temp;
            bld_file_id file_id;

            temp = path_copy(&root);
            path_append_path(&temp, include_path);

            log_error("include Checking: \"%s\"", path_to_string(&temp));
            file_id = file_get_id(&temp);
            log_warn("checked");
            set_add(&resolved_includes, file_id, include_path);

            path_free(&temp);
        }

        *includes = resolved_includes;

        path_free(&root);
    }

    array_free(&includes_array);
    return 0;
}

int parse_file_include(FILE* file, bld_array* array) {
    int error;
    bld_string str;
    bld_path path;

    error = string_parse(file, &str);
    if (error) {return -1;}

    path = path_from_string(string_unpack(&str));
    string_free(&str);

    array_push(array, &path);
    return 0;
}

int parse_file_defined_symbols(FILE* file, bld_parsing_file* f) {
    bld_set* defined;
    int amount_parsed;

    if (f->file.type != BLD_FILE_IMPLEMENTATION) {
        log_warn("Could not parse defined symbols, file type cannot defined contain symbols");
        return -1;
    }

    defined = file_defined_get(&f->file);
    if (defined == NULL) {
        log_fatal(LOG_FATAL_PREFIX "file has incorrect type");
    }

    amount_parsed = json_parse_array(file, defined, (bld_parse_func) parse_file_function);
    if (amount_parsed < 0) {
        bld_iter iter;
        bld_string* flag;

        iter = iter_set(defined);
        while (iter_next(&iter, (void**) &flag)) {
            string_free(flag);
        }
        set_free(defined);

        log_warn("Could not parse defined symbols");
        return -1;
    }

    return 0;
}

int parse_file_undefined_symbols(FILE* file, bld_parsing_file* f) {
    bld_set* undefined;
    int amount_parsed;

    if (f->file.type != BLD_FILE_IMPLEMENTATION && f->file.type != BLD_FILE_TEST) {
        log_fatal("Could not parse undefined symbols, file type cannot contain undefined symbols");
        return -1;
    }

    undefined = file_undefined_get(&f->file);
    if (undefined == NULL) {
        log_fatal("parse_file_undefined_symbols: unreachable error");
    }

    amount_parsed = json_parse_array(file, undefined, (bld_parse_func) parse_file_function);
    if (amount_parsed < 0) {
        bld_iter iter;
        bld_string* flag;

        iter = iter_set(undefined);
        while (iter_next(&iter, (void**) &flag)) {
            string_free(flag);
        }
        set_free(undefined);

        log_warn("Could not parse undefined symbols");
        return -1;
    }

    return 0;
}

int parse_file_function(FILE* file, bld_set* set) {
    bld_string str;
    int error;

    error = string_parse(file, &str);
    if (error) {return -1;}

    set_add(set, string_hash(string_unpack(&str)), &str);
    return 0;
}

int parse_file_sub_files(FILE* file, bld_parsing_file* f) {
    int amount_parsed;

    if (f->file.type != BLD_FILE_DIRECTORY) {
        log_warn("parse_file_sub_files: attempting to parse sub files to file which is not a directory");
        return -1;
    }

    f->file.info.dir.files = array_new(sizeof(uintmax_t));
    amount_parsed = json_parse_array(file, f, (bld_parse_func) parse_file_sub_file);
    if (amount_parsed < 0) {
        return -1;
    }
    return 0;
}

int parse_file_sub_file(FILE* file, bld_parsing_file* f) {
    int error;
    bld_parsing_file sub_file;

    sub_file.cache = f->cache;
    sub_file.parent = f->file.identifier.id;
    sub_file.is_rebuild_main = 0;
    error = parse_file(file, &sub_file);
    if (error) {
        return -1;
    }

    file_dir_add_file(&f->file, &sub_file.file);
    return 0;
}

int parse_project_rebuild_main(FILE* file, bld_project_cache* cache) {
    int error;
    bld_file* root;
    bld_parsing_file f;

    if (!cache->base->rebuilding) {
        log_error(LOG_FATAL_PREFIX "cache has rebuild main but is not rebuilding");
        return -1;
    }

    if (cache->root_file == BLD_INVALID_IDENITIFIER) {
        return -1;
    }

    root = set_get(&cache->files, cache->root_file);
    if (root == NULL) {
        log_error(LOG_FATAL_PREFIX "unreachable error");
        return -1;
    }

    f.cache = cache;
    f.parent = root->identifier.id;
    f.parent_path = NULL;
    f.is_rebuild_main = 1;
    error = parse_file(file, &f);
    if (error) {
        return -1;
    }

    file_dir_add_file(root, &f.file);
    return 0;
}
