#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include "logging.h"
#include "path.h"
#include "project.h"
#include "json.h"

void ensure_directory_exists(bld_path*);
int parse_cache(bld_project_cache*, bld_path*);
int parse_project_compiler(FILE*, bld_project_cache*);
int parse_project_linker(FILE*, bld_project_cache*);

int parse_project_files(FILE*, bld_project_cache*);
int parse_file(FILE*, bld_project_cache*);
int parse_file_type(FILE*, bld_project_cache*);
int parse_file_id(FILE*, bld_project_cache*);
int parse_file_hash(FILE*, bld_project_cache*);
int parse_file_name(FILE*, bld_project_cache*);
int parse_file_compiler(FILE*, bld_project_cache*);
int parse_file_linker_flags(FILE*, bld_project_cache*);
int parse_file_defined_symbols(FILE*, bld_project_cache*);
int parse_file_undefined_symbols(FILE*, bld_project_cache*);
int parse_file_function(FILE*, bld_set*);
int parse_file_includes(FILE*, bld_project_cache*);
int parse_file_include(FILE*, bld_set*);

int parse_compiler(FILE*, bld_compiler*);
int parse_compiler_type(FILE*, bld_compiler*);
int parse_compiler_executable(FILE*, bld_compiler*);
int parse_compiler_flags(FILE*, bld_compiler*);
int parse_compiler_option(FILE*, bld_array*);

int parse_linker(FILE*, bld_linker*);
int parse_linker_executable(FILE*, bld_linker*);
int parse_linker_linker_flags(FILE*, bld_linker*);
int parse_linker_flags(FILE*, bld_linker_flags*);
int parse_linker_flag(FILE*, bld_linker_flags*);

int parse_uintmax_array(FILE*, bld_array*);
int parse_uintmax_set(FILE*, bld_set*);

void ensure_directory_exists(bld_path* directory_path) {
    errno = 0;
    mkdir(path_to_string(directory_path), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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
    fproject->base.cache.file_compilers = array_new(sizeof(bld_compiler));

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
    int size = 3;
    int parsed[3];
    char *keys[3] = {"compiler", "linker", "files"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_project_compiler,
        (bld_parse_func) parse_project_linker,
        (bld_parse_func) parse_project_files,
    };
    bld_path path = path_copy(root);
    FILE* f;

    path_append_path(&path, &cache->root);
    path_append_string(&path, BLD_CACHE_NAME);
    f = fopen(path_to_string(&path), "r");

    cache->file_compilers = array_new(sizeof(bld_compiler));
    cache->file_linker_flags = array_new(sizeof(bld_linker_flags));
    amount_parsed = parse_map(f, cache, size, parsed, keys, funcs);

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
    int amount_parsed;

    cache->files = set_new(sizeof(bld_file));
    amount_parsed = parse_array(file, cache, (bld_parse_func) parse_file);
    if (amount_parsed < 0) {
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

        log_warn("Could not parse graph");
        return -1;
    }

    return 0;
}

int parse_file(FILE* file, bld_project_cache* cache) {
    bld_file temp, *f;
    int amount_parsed;
    int size = 9;
    int parsed[9];
    char *keys[9] = {"type", "id", "hash", "name", "compiler", "linker_flags", "includes", "defined_symbols", "undefined_symbols"};
    bld_parse_func funcs[9] = {
        (bld_parse_func) parse_file_type,
        (bld_parse_func) parse_file_id,
        (bld_parse_func) parse_file_hash,
        (bld_parse_func) parse_file_name,
        (bld_parse_func) parse_file_compiler,
        (bld_parse_func) parse_file_linker_flags,
        (bld_parse_func) parse_file_includes,
        (bld_parse_func) parse_file_defined_symbols,
        (bld_parse_func) parse_file_undefined_symbols,
    };

    temp.path = path_new();
    temp.compiler = -1;
    temp.defined_symbols = set_new(sizeof(char*));
    temp.undefined_symbols = set_new(sizeof(char*));
    set_add(&cache->files, BLD_INVALID_IDENITIFIER, &temp);
    f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);

    amount_parsed = parse_map(file, cache, size, parsed, keys, funcs);
    f = set_remove(&cache->files, BLD_INVALID_IDENITIFIER);

    if (amount_parsed < 0) {goto parse_failed;}
    if (!parsed[0]) {goto parse_failed;} /* Fail if file type was not parsed */

    if (f->type == BLD_HEADER) {
        /* Header must have: id, hash, name, includes */
        if (!parsed[1] || !parsed[2] || !parsed[3] || !parsed[6]) {
            log_warn("Header did not have required fields");
            goto parse_failed;
        }
        /* Header cannot have: defined_symbols, undefined_symbols */
        if (parsed[7] || parsed[8]) {
            log_warn("Header had forbidden fields");
            goto parse_failed;
        }
    } else {
        /* File must have every key except an optional compiler */
        if ((amount_parsed < size - 2) || ((amount_parsed == size - 1) && parsed[4] && parsed[5])) {
            log_warn("File is missing required field");
            goto parse_failed;
        }
    }

    set_add(&cache->files, f->identifier.id, f);

    return 0;

    parse_failed:
    path_free(&f->path);
    if (parsed[3]) {
        string_free(&f->name);
    }

    if (parsed[6]) {
        set_free(&f->includes);
    }

    if (parsed[7]) {
        bld_iter iter = iter_set(&f->defined_symbols);
        char** str;

        while (iter_next(&iter, (void**) &str)) {
            free(*str);
        }
        set_free(&f->defined_symbols);
    }

    if (parsed[8]) {
        bld_iter iter = iter_set(&f->undefined_symbols);
        char** str;

        while (iter_next(&iter, (void**) &str)) {
            free(*str);
        }
        set_free(&f->undefined_symbols);
    }

    return -1;
}

int parse_file_type(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);

    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler type");
        return -1;
    }
    
    temp = string_unpack(&str);
    if (strcmp(temp, "implementation") == 0) {
        f->type = BLD_IMPL;
    } else if (strcmp(temp, "header") == 0) {
        f->type = BLD_HEADER;
    } else if (strcmp(temp, "test") == 0) {
        f->type = BLD_TEST;
    } else {
        log_warn("Not a valid file type: \"%s\"", temp);
        result = -1;
    }

    string_free(&str);
    return result;
}

int parse_file_id(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    uintmax_t num;
    int result = parse_uintmax(file, &num);
    if (result) {
        log_warn("Could not parse file id");
        return -1;
    }
    f->identifier.id = num;
    return 0;
}

int parse_file_hash(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    uintmax_t num;
    int result = parse_uintmax(file, &num);
    if (result) {
        log_warn("Could not parse file id");
        return -1;
    }
    f->identifier.hash = num;
    return 0;
}

int parse_file_name(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    bld_string str = string_new();
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse file name");
        return -1;
    }
    
    f->name = str;
    return result;
}

int parse_file_compiler(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    bld_compiler temp;
    int result;

    result = parse_compiler(file, &temp);
    if (result) {log_warn("Could not parse compiler for file.");}

    array_push(&cache->file_compilers, &temp);
    f->compiler = cache->file_compilers.size - 1;

    return result;
}

int parse_file_linker_flags(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    bld_linker_flags temp;
    int result;

    temp = linker_flags_new();
    result = parse_linker_flags(file, &temp);
    if (result) {log_warn("Could not parse linker flags for file");}

    array_push(&cache->file_linker_flags, &temp);
    f->linker_flags = cache->file_linker_flags.size - 1;

    return result;
}

int parse_file_includes(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    int amount_parsed;

    f->includes = set_new(0);
    amount_parsed = parse_array(file, &f->includes, (bld_parse_func) parse_file_include);
    if (amount_parsed < 0) {
        set_free(&f->includes);
        log_warn("Could not parse file includes");
        return -1;
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

int parse_file_defined_symbols(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    int amount_parsed;

    amount_parsed = parse_array(file, &f->defined_symbols, (bld_parse_func) parse_file_function);
    if (amount_parsed < 0) {
        bld_iter iter = iter_set(&f->defined_symbols);
        char** flag;

        while (iter_next(&iter, (void**) &flag)) {
            free(*flag);
        }
        set_free(&f->defined_symbols);

        log_warn("Could not parse defined symbols");
        return -1;
    }

    return 0;
}

int parse_file_undefined_symbols(FILE* file, bld_project_cache* cache) {
    bld_file* f = set_get(&cache->files, BLD_INVALID_IDENITIFIER);
    int amount_parsed;

    amount_parsed = parse_array(file, &f->undefined_symbols, (bld_parse_func) parse_file_function);
    if (amount_parsed < 0) {
        bld_iter iter = iter_set(&f->undefined_symbols);
        char** flag;

        while (iter_next(&iter, (void**) &flag)) {
            free(*flag);
        }
        set_free(&f->undefined_symbols);

        log_warn("Could not parse defined symbols");
        return -1;
    }

    return 0;
}

int parse_file_function(FILE* file, bld_set* set) {
    bld_string str = string_new();
    char* temp;
    int result;

    result = parse_string(file, &str);
    if (result) {string_free(&str); return -1;}

    temp = string_unpack(&str);
    set_add(set, string_hash(temp, 0), &temp);
    return 0;
}

int parse_compiler(FILE* file, bld_compiler* compiler) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"executable", "flags"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_compiler_executable,
        (bld_parse_func) parse_compiler_flags,
    };

    compiler->flags = array_new(sizeof(char*));
    amount_parsed = parse_map(file, compiler, size, parsed, (char**) keys, funcs);

    if (amount_parsed < size && !parsed[0]) {
        return -1;
    }
    return 0;
}

int parse_compiler_executable(FILE* file, bld_compiler* compiler) {
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler executable");
        return -1;
    }
    
    temp = string_unpack(&str);
    compiler->executable = temp;
    return result;
}

int parse_compiler_flags(FILE* file, bld_compiler* compiler) {
    int values;
    bld_iter iter;
    char** flag;
    bld_array flags = array_new(sizeof(char*));

    values = parse_array(file, &flags, (bld_parse_func) parse_compiler_option);
    if (values < 0) {
        iter = iter_array(&flags);
        while (iter_next(&iter, (void**) &flag)) {
            free(*flag);
        }
        array_free(&flags);

        log_warn("Could not parse compiler flags");
        return -1;
    }

    compiler->flags = flags;
    return 0;
}

int parse_compiler_option(FILE* file, bld_array* flags) {
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler flag");
        return -1;
    }
    
    temp = string_unpack(&str);
    array_push(flags, &temp);

    return result;
}

int parse_linker(FILE* file, bld_linker* linker) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"executable", "linker_flags"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_linker_executable,
        (bld_parse_func) parse_linker_linker_flags,
    };

    linker->flags = linker_flags_new();
    amount_parsed = parse_map(file, linker, size, parsed, (char**) keys, funcs);

    if (amount_parsed < size && !parsed[0]) {
        return -1;
    }
    return 0;
}

int parse_linker_executable(FILE* file, bld_linker* linker) {
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);

    if (result) {
        string_free(&str);
        log_warn("Could not parse linker executable");
        return -1;
    }

    temp = string_unpack(&str);
    linker->executable = temp;
    return result;
}

int parse_linker_linker_flags(FILE* file, bld_linker* linker) {
    bld_linker_flags flags = linker_flags_new();
    int result = parse_linker_flags(file, &flags);
    if (result) {
        linker_flags_free(&flags);
        log_warn("Could not parse linker flags");
    }

    linker->flags = flags;
    return result;
}

int parse_linker_flags(FILE* file, bld_linker_flags* flags) {
    int values;

    values = parse_array(file, flags, (bld_parse_func) parse_linker_flag);
    if (values < 0) {
        log_warn("Could not parse linker flags");
        return -1;
    }

    return 0;
}

int parse_linker_flag(FILE* file, bld_linker_flags* flags) {
    bld_string str = string_new();
    char* temp;
    uintmax_t temp_hash;
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse linker flag");
        return -1;
    }

    temp = string_unpack(&str);
    temp_hash = string_hash(temp, 76502);  /* Magic number from linker_flags_add_flag */
    array_push(&flags->flag, &temp);
    array_push(&flags->hash, &temp_hash);

    return result;
}
