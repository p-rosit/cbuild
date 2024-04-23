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
} bld_parsing_file;

typedef enum bld_file_fields {
    BLD_PARSE_TYPE = 0,
    BLD_PARSE_ID = 1,
    BLD_PARSE_MTIME = 2,
    BLD_PARSE_HASH = 3,
    BLD_PARSE_NAME = 4,
    BLD_PARSE_COMPILER = 5,
    BLD_PARSE_LINKER = 6,
    BLD_PARSE_INCLUDES = 7,
    BLD_PARSE_DEFINED = 8,
    BLD_PARSE_UNDEFINED = 9,
    BLD_PARSE_FILES = 10,
    BLD_TOTAL_FIELDS = 11
} bld_file_fields;

typedef struct bld_parsing_linker_flags {
    bld_set file2linker_flags;
    bld_array linker_flags;
} bld_parsing_linker_flags;

typedef struct bld_parsing_file_linker_flags {
    uintmax_t file_id;
    bld_linker_flags flags;
} bld_parsing_file_linker_flags;

void ensure_directory_exists(bld_path*);
int parse_cache(bld_project_cache*, bld_path*);
int parse_project_compiler(FILE*, bld_project_cache*);
int parse_project_linker(FILE*, bld_project_cache*);

int parse_project_files(FILE*, bld_project_cache*);
int parse_file(FILE*, bld_parsing_file*);
int parse_file_type(FILE*, bld_parsing_file*);
int parse_file_id(FILE*, bld_parsing_file*);
int parse_file_mtime(FILE*, bld_parsing_file*);
int parse_file_hash(FILE*, bld_parsing_file*);
int parse_file_name(FILE*, bld_parsing_file*);
int parse_file_compiler(FILE*, bld_parsing_file*);
int parse_file_defined_symbols(FILE*, bld_parsing_file*);
int parse_file_undefined_symbols(FILE*, bld_parsing_file*);
int parse_file_function(FILE*, bld_set*);
int parse_file_includes(FILE*, bld_parsing_file*);
int parse_file_include(FILE*, bld_set*);
int parse_file_sub_files(FILE*, bld_parsing_file*);

int parse_project_linker_flags(FILE*, bld_project_cache*);
int parse_file_linker_flags(FILE*, bld_parsing_linker_flags*);
int parse_file_linker_flags_id(FILE*, bld_parsing_file_linker_flags*);
int parse_file_linker_flags_linker_flags(FILE*, bld_parsing_file_linker_flags*);

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
    fproject->base.cache.file2compiler = set_new(sizeof(size_t));
    fproject->base.cache.file_compilers = array_new(sizeof(bld_compiler_or_flags));
    fproject->base.cache.file2linker_flags = set_new(sizeof(size_t));
    fproject->base.cache.file_linker_flags = array_new(sizeof(bld_linker_flags));
    fproject->base.cache.files = set_new(sizeof(bld_file));
    fproject->base.cache.tree = file_tree_new();

    if (file == NULL) {
        log_debug("No cache file found.");
    } else {
        int result;

        fclose(file);
        log_debug("Found cache file, attempting to parse.");
        result = parse_cache(&fproject->base.cache, &fproject->base.root);

        if (result) {
            log_warn("Could not parse cache, ignoring.");
        } else {
            log_info("Loaded cache.");
            fproject->base.cache.set = 1;
        }
    }

    path_free(&path);
}

int parse_cache(bld_project_cache* cache, bld_path* root) {
    int amount_parsed;
    int size = 4;
    int parsed[4];
    char *keys[4] = {"compiler", "linker", "files", "file_linker_flags"};
    bld_parse_func funcs[4] = {
        (bld_parse_func) parse_project_compiler,
        (bld_parse_func) parse_project_linker,
        (bld_parse_func) parse_project_files,
        (bld_parse_func) parse_project_linker_flags,
    };
    bld_path path = path_copy(root);
    FILE* f;

    path_append_path(&path, &cache->root);
    path_append_string(&path, BLD_CACHE_NAME);
    f = fopen(path_to_string(&path), "r");

    amount_parsed = json_parse_map(f, cache, size, parsed, keys, funcs);

    if (amount_parsed != size) {
        fclose(f);
        path_free(&path);

        if (parsed[0]) {
            compiler_free(&cache->compiler);
        }

        if (parsed[1]) {
            linker_free(&cache->linker);
        }

        if (parsed[2]) {
            bld_iter iter;
            bld_file* file;
            bld_compiler_or_flags* compiler;

            iter = iter_set(&cache->files);
            while (iter_next(&iter, (void**) &file)) {
                file_free(file);
            }
            set_free(&cache->files);

            iter = iter_array(&cache->file_compilers);
            while (iter_next(&iter, (void**) &compiler)) {
                switch (compiler->type) {
                    case (BLD_COMPILER): {
                        compiler_free(&compiler->as.compiler);
                    } break;
                    case (BLD_COMPILER_FLAGS): {
                        compiler_flags_free(&compiler->as.flags);
                    } break;
                    default: log_fatal("parse_cache: internal error");
                }
            }
            array_free(&cache->file_compilers);
        }

        if (parsed[3]) {
            bld_iter iter;
            bld_linker_flags* flags;

            iter = iter_array(&cache->file_linker_flags);
            while (iter_next(&iter, (void**) &flags)) {
                linker_flags_free(flags);
            }
            array_free(&cache->file_linker_flags);
            set_free(&cache->file2linker_flags);
        }

        return -1;
    }

    fclose(f);
    path_free(&path);
    return 0;
}

int parse_project_compiler(FILE* file, bld_project_cache* cache) {
    int result = parse_compiler(file, &cache->compiler);
    if (result) {log_warn("Could not parse compiler for project.");}
    return result;
}

int parse_project_linker(FILE* file, bld_project_cache* cache) {
    int result = parse_linker(file, &cache->linker);
    if (result) {log_warn("Could not parse linker for project.");}
    return result;
}

int parse_project_files(FILE* file, bld_project_cache* cache) {
    int result;
    bld_parsing_file f;

    f.cache = cache;
    f.parent = BLD_INVALID_IDENITIFIER;

    result = parse_file(file, &f);
    if (result) {
        bld_iter iter = iter_set(&cache->files);
        bld_file* file;
        bld_compiler* compiler;
        bld_linker_flags* flags;

        iter = iter_set(&cache->files);
        while (iter_next(&iter, (void**) &file)) {
            file_free(file);
        }
        set_free(&cache->files);

        iter = iter_array(&cache->file_compilers);
        while (iter_next(&iter, (void**) &compiler)) {
            compiler_free(compiler);
        }
        array_free(&cache->file_compilers);

        iter = iter_array(&cache->file_linker_flags);
        while (iter_next(&iter, (void**) &flags)) {
            linker_flags_free(flags);
        }
        array_free(&cache->file_linker_flags);

        file_tree_free(&cache->tree);
        log_warn("Could not parse file tree");
        return -1;
    }

    return 0;
}

int parse_file(FILE* file, bld_parsing_file* f) {
    int amount_parsed;
    int parsed[BLD_TOTAL_FIELDS];
    char *keys[BLD_TOTAL_FIELDS] = {
        "type",
        "id",
        "mtime",
        "hash",
        "name",
        "compiler",
        "linker_flags",
        "includes",
        "defined_symbols",
        "undefined_symbols",
        "files",
    };
    bld_parse_func funcs[BLD_TOTAL_FIELDS] = {
        (bld_parse_func) parse_file_type,
        (bld_parse_func) parse_file_id,
        (bld_parse_func) parse_file_mtime,
        (bld_parse_func) parse_file_hash,
        (bld_parse_func) parse_file_name,
        (bld_parse_func) parse_file_compiler,
        (bld_parse_func) parse_file_linker_flags,
        (bld_parse_func) parse_file_includes,
        (bld_parse_func) parse_file_defined_symbols,
        (bld_parse_func) parse_file_undefined_symbols,
        (bld_parse_func) parse_file_sub_files,
    };

    f->file.type = BLD_INVALID_FILE;
    f->file.path = path_new();
    f->file.compiler = -1;
    f->file.linker_flags = -1;

    amount_parsed = json_parse_map(file, f, BLD_TOTAL_FIELDS, parsed, keys, funcs);

    if (amount_parsed < 0) {goto parse_failed;}
    if (!parsed[BLD_PARSE_TYPE]) {goto parse_failed;} /* Fail if file type was not parsed */

    switch (f->file.type) {
        case (BLD_DIR): {
            if (
                !parsed[BLD_PARSE_ID]
                || !parsed[BLD_PARSE_NAME]
                || !parsed[BLD_PARSE_FILES]
            ) {
                log_warn("Directory requires the following fields: [\"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_ID], keys[BLD_PARSE_NAME], keys[BLD_PARSE_FILES]);
                goto parse_failed;
            }
            if (
                parsed[BLD_PARSE_MTIME]
                || parsed[BLD_PARSE_HASH]
                || parsed[BLD_PARSE_COMPILER]
                || parsed[BLD_PARSE_LINKER]
                || parsed[BLD_PARSE_INCLUDES]
                || parsed[BLD_PARSE_DEFINED]
                || parsed[BLD_PARSE_UNDEFINED]
            ) {
                log_warn("Directory cannot have any of the following fields: [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_MTIME], keys[BLD_PARSE_HASH], keys[BLD_PARSE_COMPILER], keys[BLD_PARSE_LINKER], keys[BLD_PARSE_INCLUDES], keys[BLD_PARSE_DEFINED], keys[BLD_PARSE_UNDEFINED]);
                goto parse_failed;
            }
        } break;
        case (BLD_IMPL): {
            if (
                !parsed[BLD_PARSE_ID]
                || !parsed[BLD_PARSE_MTIME]
                || !parsed[BLD_PARSE_HASH]
                || !parsed[BLD_PARSE_NAME]
                || !parsed[BLD_PARSE_INCLUDES]
                || !parsed[BLD_PARSE_DEFINED]
                || !parsed[BLD_PARSE_UNDEFINED]
            ) {
                log_warn("Implementation file requires the following fields: [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_ID], keys[BLD_PARSE_MTIME], keys[BLD_PARSE_HASH], keys[BLD_PARSE_NAME], keys[BLD_PARSE_INCLUDES], keys[BLD_PARSE_DEFINED], keys[BLD_PARSE_UNDEFINED]);
                goto parse_failed;
            }
            if (
                parsed[BLD_PARSE_FILES]
            ) {
                log_warn("Implementation file cannot have any of the following fields: [\"%s\"]", keys[BLD_PARSE_FILES]);
                goto parse_failed;
            }
        } break;
        case (BLD_TEST): {
            if (
                !parsed[BLD_PARSE_ID]
                || !parsed[BLD_PARSE_MTIME]
                || !parsed[BLD_PARSE_HASH]
                || !parsed[BLD_PARSE_NAME]
                || !parsed[BLD_PARSE_INCLUDES]
                || !parsed[BLD_PARSE_UNDEFINED]
            ) {
                log_warn("Test file requires the following fields: [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_ID], keys[BLD_PARSE_MTIME], keys[BLD_PARSE_HASH], keys[BLD_PARSE_NAME], keys[BLD_PARSE_INCLUDES], keys[BLD_PARSE_UNDEFINED]);
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
        case (BLD_HEADER): {
            if (
                !parsed[BLD_PARSE_ID]
                || !parsed[BLD_PARSE_MTIME]
                || !parsed[BLD_PARSE_HASH]
                || !parsed[BLD_PARSE_NAME]
                || !parsed[BLD_PARSE_INCLUDES]
            ) {
                log_warn("Header file requires the following fields: [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]", keys[BLD_PARSE_ID], keys[BLD_PARSE_MTIME], keys[BLD_PARSE_HASH], keys[BLD_PARSE_NAME], keys[BLD_PARSE_INCLUDES]);
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

    if (parsed[BLD_PARSE_INCLUDES]) {
        bld_set* includes;

        switch (f->file.type) {
            case (BLD_DIR): {
                goto includes_freed;
            } break;
            case (BLD_IMPL): {
                includes = &f->file.info.impl.includes;
            } break;
            case (BLD_TEST): {
                includes = &f->file.info.test.includes;
            } break;
            case (BLD_HEADER): {
                includes = &f->file.info.header.includes;
            } break;
            default: log_fatal("freeing includes: unrecognized file type");
        }

        set_free(includes);
    }
    includes_freed:

    if (parsed[BLD_PARSE_DEFINED]) {
        bld_set* defined;
        bld_iter iter;
        bld_string* str;

        defined = &f->file.info.impl.defined_symbols;

        iter = iter_set(defined);
        while (iter_next(&iter, (void**) &str)) {
            string_free(str);
        }
        set_free(defined);
    }

    if (parsed[BLD_PARSE_UNDEFINED]) {
        bld_set* undefined;
        bld_iter iter;
        bld_string* str;

        switch (f->file.type) {
            case (BLD_IMPL): {
                undefined = &f->file.info.impl.undefined_symbols;
            } break;
            case (BLD_TEST): {
                undefined = &f->file.info.test.undefined_symbols;
            } break;
            default: goto undefined_freed;
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
    int result = string_parse(file, &str);

    if (result) {
        log_warn("Could not parse compiler type");
        return -1;
    }
    
    temp = string_unpack(&str);
    if (strcmp(temp, "directory") == 0) {
        f->file.type = BLD_DIR;
    } else if (strcmp(temp, "implementation") == 0) {
        f->file.type = BLD_IMPL;
        f->file.info.impl.undefined_symbols = set_new(sizeof(bld_string));
        f->file.info.impl.defined_symbols = set_new(sizeof(bld_string));
    } else if (strcmp(temp, "header") == 0) {
        f->file.type = BLD_HEADER;
    } else if (strcmp(temp, "test") == 0) {
        f->file.type = BLD_TEST;
        f->file.info.test.undefined_symbols = set_new(sizeof(bld_string));
    } else {
        log_warn("Not a valid file type: \"%s\"", temp);
        result = -1;
    }

    string_free(&str);
    return result;
}

int parse_file_id(FILE* file, bld_parsing_file* f) {
    uintmax_t num;
    int result = parse_uintmax(file, &num);
    if (result) {
        log_warn("Could not parse file id");
        return -1;
    }
    f->file.identifier.id = num;

    if (f->parent != BLD_INVALID_IDENITIFIER) {
        file_tree_add(&f->cache->tree, f->parent, f->file.identifier.id);
    } else {
        file_tree_set_root(&f->cache->tree, f->file.identifier.id);
    }
    return 0;
}

int parse_file_mtime(FILE* file, bld_parsing_file* f) {
    uintmax_t num;
    int result = parse_uintmax(file, &num);
    if (result) {
        log_warn("Could not parse file mtime");
        return -1;
    }
    f->file.identifier.time = num;
    return 0;
}

int parse_file_hash(FILE* file, bld_parsing_file* f) {
    uintmax_t num;
    int result = parse_uintmax(file, &num);
    if (result) {
        log_warn("Could not parse file id");
        return -1;
    }
    f->file.identifier.hash = num;
    return 0;
}

int parse_file_name(FILE* file, bld_parsing_file* f) {
    bld_string str;
    int result = string_parse(file, &str);
    if (result) {
        log_warn("Could not parse file name");
        return -1;
    }
    
    f->file.name = str;
    return result;
}

int parse_file_compiler(FILE* file, bld_parsing_file* f) {
    bld_compiler temp;
    bld_compiler_or_flags cf;
    int result;
    size_t index;

    result = parse_compiler(file, &temp);
    if (result) {log_warn("Could not parse compiler for file.");}

    cf.type = BLD_COMPILER;
    cf.as.compiler = temp;

    array_push(&f->cache->file_compilers, &cf);
    index = f->cache->file_compilers.size - 1;
    f->file.compiler = index;
    set_add(&f->cache->file2compiler, f->file.identifier.id, &index);

    return result;
}

int parse_file_includes(FILE* file, bld_parsing_file* f) {
    int amount_parsed;
    bld_set includes;

    includes = set_new(0);
    amount_parsed = json_parse_array(file, &includes, (bld_parse_func) parse_file_include);
    if (amount_parsed < 0) {
        set_free(&includes);
        log_warn("Could not parse file includes");
        return -1;
    }

    switch (f->file.type) {
        case (BLD_DIR): {
            set_free(&includes);
            return -1;
        } break;
        case (BLD_IMPL): {
            f->file.info.impl.includes = includes;
        } break;
        case (BLD_TEST): {
            f->file.info.test.includes = includes;
        } break;
        case (BLD_HEADER): {
            f->file.info.header.includes = includes;
        } break;
        default: log_fatal("parse_file_includes: internal error");
    }

    return 0;
}

int parse_file_include(FILE* file, bld_set* set) {
    uintmax_t file_id;
    int result;

    result = parse_uintmax(file, &file_id);
    if (result) {return -1;}

    set_add(set, file_id, NULL);
    return 0;
}

int parse_file_defined_symbols(FILE* file, bld_parsing_file* f) {
    bld_set* defined;
    int amount_parsed;

    if (f->file.type != BLD_IMPL) {
        log_warn("Could not parse defined symbols, file type cannot defined contain symbols");
        return -1;
    }

    defined = &f->file.info.impl.defined_symbols;
    amount_parsed = json_parse_array(file, defined, (bld_parse_func) parse_file_function);
    if (amount_parsed < 0) {
        bld_iter iter = iter_set(defined);
        bld_string* flag;

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

    if (f->file.type != BLD_IMPL && f->file.type != BLD_TEST) {
        log_fatal("Could not parse undefined symbols, file type cannot contain undefined symbols");
        return -1;
    }

    switch (f->file.type) {
        case (BLD_IMPL): {
            undefined = &f->file.info.impl.undefined_symbols;
        } break;
        case (BLD_TEST): {
            undefined = &f->file.info.test.undefined_symbols;
        } break;
        default: log_fatal("parse_file_undefined_symbols: unreachable error");
    }

    amount_parsed = json_parse_array(file, undefined, (bld_parse_func) parse_file_function);
    if (amount_parsed < 0) {
        bld_iter iter = iter_set(undefined);
        bld_string* flag;

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
    int result;

    result = string_parse(file, &str);
    if (result) {return -1;}

    set_add(set, string_hash(string_unpack(&str)), &str);
    return 0;
}

int parse_file_sub_files(FILE* file, bld_parsing_file* f) {
    int amount_parsed;
    bld_parsing_file child;

    child.cache = f->cache;
    child.parent = f->file.identifier.id;
    amount_parsed = json_parse_array(file, &child, (bld_parse_func) parse_file);
    if (amount_parsed < 0) {
        return -1;
    }

    return 0;
}

int parse_project_linker_flags(FILE* file, bld_project_cache* cache) {
    int amount_parsed;
    bld_parsing_linker_flags temp_flags;

    temp_flags.file2linker_flags = set_new(sizeof(size_t));
    temp_flags.linker_flags = array_new(sizeof(bld_linker_flags));
    amount_parsed = json_parse_array(file, &temp_flags, (bld_parse_func) parse_file_linker_flags);
    if (amount_parsed < 0) {
        bld_iter iter;
        bld_linker_flags* flags;

        iter = iter_array(&temp_flags.linker_flags);
        while (iter_next(&iter, (void**) &flags)) {
            linker_flags_free(flags);
        }
        array_free(&temp_flags.linker_flags);
        set_free(&temp_flags.file2linker_flags);
        return -1;
    }

    cache->file2linker_flags = temp_flags.file2linker_flags;
    cache->file_linker_flags = temp_flags.linker_flags;
    return 0;
}

int parse_file_linker_flags(FILE* file, bld_parsing_linker_flags* flags) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"file_id", "linker_flags"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_file_linker_flags_id,
        (bld_parse_func) parse_file_linker_flags_linker_flags,
    };
    size_t index;
    bld_parsing_file_linker_flags file_flags;

    amount_parsed = json_parse_map(file, &file_flags, size, parsed, keys, funcs);

    if (amount_parsed != size) {
        if (parsed[1]) {
            linker_flags_free(&file_flags.flags);
        }
        return -1;
    }

    index = flags->linker_flags.size;

    set_add(&flags->file2linker_flags, file_flags.file_id, &index);
    array_push(&flags->linker_flags, &file_flags.flags);
    return 0;
}

int parse_file_linker_flags_id(FILE* file, bld_parsing_file_linker_flags* flags) {
    uintmax_t file_id;
    int result = parse_uintmax(file, &file_id);
    if (result) {
        return -1;
    }

    flags->file_id = file_id;
    return 0;
}

int parse_file_linker_flags_linker_flags(FILE* file, bld_parsing_file_linker_flags* flags) {
    bld_linker_flags file_flags = linker_flags_new();
    int result = parse_linker_flags(file, &file_flags);
    if (result) {
        return -1;
    }

    flags->flags = file_flags;

    return 0;
}
