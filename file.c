#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "logging.h"
#include "file.h"

bld_file_identifier get_identifier(bld_path*);
bld_file make_file(bld_file_type, bld_path*, char*);
bld_set file_copy_symbol_set(bld_set*);

uintmax_t file_get_id(bld_path* path) {
    struct stat file_stat;

    if (stat(path_to_string(path), &file_stat) < 0) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
    }

    return file_stat.st_ino;
}

bld_file_identifier get_identifier(bld_path* path) {
    struct stat file_stat;

    if (stat(path_to_string(path), &file_stat) < 0) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
    }
    return (bld_file_identifier) {
        .id = file_stat.st_ino,
        .time = file_stat.st_mtime,
        .hash = 0,
    };
}

int file_eq(bld_file* f1, bld_file* f2) {
    return f1->identifier.id == f2->identifier.id;
}

void serialize_identifier(char name[FILENAME_MAX], bld_file* file) {
    sprintf(name, "%ju", (uintmax_t) file->identifier.id);
}

bld_file make_file(bld_file_type type, bld_path* path, char* name) {
    bld_string str = string_new();
    string_append_string(&str, name);
    return (bld_file) {
        .type = type,
        .identifier = get_identifier(path),
        .name = str,
        .path = *path,
        .compiler = -1,
        .defined_symbols = set_new(sizeof(char*)),
        .undefined_symbols = set_new(sizeof(char*)),
        .includes = set_new(0),
    };
}

bld_file file_header_new(bld_path* path, char* name) {
    bld_file header = make_file(BLD_HEADER, path, name);
    return header;
}

bld_file file_impl_new(bld_path* path, char* name) {
    bld_file impl = make_file(BLD_IMPL, path, name);
    return impl;
}

bld_file file_test_new(bld_path* path, char* name) {
    bld_file test = make_file(BLD_TEST, path, name);
    return test;
}

void file_free(bld_file* file) {
    char** symbol;

    path_free(&file->path);
    string_free(&file->name);

    bld_iter iter_defined = iter_set(&file->defined_symbols);
    while (iter_next(&iter_defined, (void**) &symbol)) {
        free(*symbol);
    }
    set_free(&file->defined_symbols);

    bld_iter iter_undefined = iter_set(&file->undefined_symbols);
    while (iter_next(&iter_undefined, (void**) &symbol)) {
        free(*symbol);
    }
    set_free(&file->undefined_symbols);
    set_free(&file->includes);
}

uintmax_t file_hash(bld_file* file, bld_array* compilers, uintmax_t seed) {
    seed = (seed << 3) + file->identifier.id;
    seed = (seed << 4) + seed + file->identifier.time;
    if (file->compiler > 0) {
        seed = compiler_hash(array_get(compilers, file->compiler), seed);
    }
    return seed;
}

void file_symbols_copy(bld_file* file, bld_set* defined, bld_set* undefined) {
    file->defined_symbols = file_copy_symbol_set(defined);
    file->undefined_symbols = file_copy_symbol_set(undefined);
}

bld_set file_copy_symbol_set(bld_set* set) {
    bld_iter iter;
    bld_set cpy;
    bld_string str;
    char** symbol;

    cpy = set_copy(set);
    iter = iter_set(&cpy);
    while (iter_next(&iter, (void**) &symbol)) {
        str = string_new();
        string_append_string(&str, *symbol);
        *symbol = string_unpack(&str);
    }

    return cpy;
}
