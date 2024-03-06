#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "logging.h"
#include "file.h"

bld_file_identifier get_identifier(bld_stat* file) {
    return (bld_file_identifier) {
        .id = file->st_ino,
        .time = file->st_mtime,
        .hash = 0,
    };
}

int file_eq(bld_file* f1, bld_file* f2) {
    return f1->identifier.id == f2->identifier.id;
}

void serialize_identifier(char name[256], bld_file* file) {
    sprintf(name, "%ju", (uintmax_t) file->identifier.id);
}

bld_file make_file(bld_file_type type, bld_path* path, char* name, bld_stat* stat) {
    bld_string str = new_string();
    append_string(&str, name);
    return (bld_file) {
        .type = type,
        .identifier = get_identifier(stat),
        .name = str,
        .path = *path,
        .compiler = NULL,
    };
}

bld_file make_header(bld_path* path, char* name, bld_stat* stat) {
    bld_file header = make_file(BLD_HEADER, path, name, stat);
    return header;
}

bld_file make_impl(bld_path* path, char* name, bld_stat* stat) {
    bld_file impl = make_file(BLD_IMPL, path, name, stat);
    return impl;
}

bld_file make_test(bld_path* path, char* name, bld_stat* stat) {
    bld_file test = make_file(BLD_TEST, path, name, stat);
    return test;
}

void free_file(bld_file* file) {
    free_path(&file->path);
    free_string(&file->name);
    free_compiler(file->compiler);
    free(file->compiler);
}

uintmax_t hash_file(bld_file* file, uintmax_t seed) {
    seed = (seed << 3) + file->identifier.id;
    seed = (seed << 4) + seed + file->identifier.time;
    if (file->compiler != NULL) {
        seed = hash_compiler(file->compiler, seed);
    }
    return seed;
}

bld_files new_files() {
    return (bld_files) {.set = bld_set_new()};
}

void clear_files(bld_files* files) {
    bld_set_clear(&files->set);
}

void free_files(bld_files* files) {
    bld_iter iter = bld_iter_set(&files->set, sizeof(bld_file));
    bld_file* file;

    while (bld_set_next(&iter, (void**) &file)) {
        free_file(file);
    }
    bld_set_free(&files->set);
}

int append_file(bld_files* files, bld_file file) {
    return bld_set_add(&files->set, file.identifier.id, &file, sizeof(bld_file));
}
