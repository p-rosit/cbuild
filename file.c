#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "os.h"
#include "logging.h"
#include "iter.h"
#include "file.h"

bld_file_identifier get_identifier(bld_path*);
bld_file make_file(bld_file_type, bld_path*, char*);
bld_set file_copy_symbol_set(bld_set*);

uintmax_t file_get_id(bld_path* path) {
    uintmax_t id = os_info_id(path_to_string(path));

    if (id == BLD_INVALID_IDENITIFIER) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
    }

    return id;
}

bld_file_identifier get_identifier(bld_path* path) {
    bld_file_identifier identifier;
    uintmax_t id, mtime;

    id = os_info_id(path_to_string(path));
    mtime = os_info_mtime(path_to_string(path));

    if (id == BLD_INVALID_IDENITIFIER) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
    }

    identifier.id = id;
    identifier.time = mtime; 
    identifier.hash = 0;

    return identifier;
}

int file_eq(bld_file* f1, bld_file* f2) {
    return f1->identifier.id == f2->identifier.id;
}

void serialize_identifier(char name[FILENAME_MAX], bld_file* file) {
    sprintf(name, "%" PRIuMAX, (uintmax_t) file->identifier.id);
}

bld_file make_file(bld_file_type type, bld_path* path, char* name) {
    bld_file file;
    bld_string str = string_new();
    string_append_string(&str, name);

    file.type = type;
    file.identifier = get_identifier(path);
    file.name = str;
    file.path = *path;
    file.compiler = -1;
    file.linker_flags = -1;
    file.defined_symbols = set_new(sizeof(bld_string));
    file.undefined_symbols = set_new(sizeof(bld_string));
    file.includes = set_new(0);

    return file;
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
    bld_iter iter;
    bld_string* symbol;

    path_free(&file->path);
    string_free(&file->name);

    iter = iter_set(&file->defined_symbols);
    while (iter_next(&iter, (void**) &symbol)) {
        string_free(symbol);
    }
    set_free(&file->defined_symbols);

    iter = iter_set(&file->undefined_symbols);
    while (iter_next(&iter, (void**) &symbol)) {
        string_free(symbol);
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
    bld_string str, *symbol;

    cpy = set_copy(set);
    iter = iter_set(&cpy);
    while (iter_next(&iter, (void**) &symbol)) {
        str = string_copy(symbol);
        *symbol = str;
    }

    return cpy;
}
