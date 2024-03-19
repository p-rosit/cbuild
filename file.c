#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "logging.h"
#include "file.h"

uintmax_t get_file_id(bld_path* path) {
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
        .compiler = NULL,
        .defined_symbols = set_new(sizeof(char*)),
        .undefined_symbols = set_new(sizeof(char*)),
        .includes = set_new(0),
    };
}

bld_file file_header_new(bld_path* path, char* name) {
    bld_file header = make_file(BLD_HEADER, path, name);
    return header;
}

bld_file make_impl(bld_path* path, char* name) {
    bld_file impl = make_file(BLD_IMPL, path, name);
    return impl;
}

bld_file make_test(bld_path* path, char* name) {
    bld_file test = make_file(BLD_TEST, path, name);
    return test;
}

void free_file(bld_file* file) {
    char** symbol;

    path_free(&file->path);
    string_free(&file->name);
    compiler_free(file->compiler);
    free(file->compiler);

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

uintmax_t hash_file(bld_file* file, uintmax_t seed) {
    seed = (seed << 3) + file->identifier.id;
    seed = (seed << 4) + seed + file->identifier.time;
    if (file->compiler != NULL) {
        seed = compiler_hash(file->compiler, seed);
    }
    return seed;
}

bld_files new_files() {
    return (bld_files) {.set = set_new(sizeof(bld_file))};
}

void clear_files(bld_files* files) {
    set_clear(&files->set);
}

void free_files(bld_files* files) {
    bld_iter iter = iter_set(&files->set);
    bld_file* file;

    while (iter_next(&iter, (void**) &file)) {
        free_file(file);
    }
    set_free(&files->set);
}

int append_file(bld_files* files, bld_file file) {
    return set_add(&files->set, file.identifier.id, &file);
}
