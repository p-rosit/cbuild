#include <string.h>
#include "build.h"
#include "file.h"

bld_string extract_name(bld_dirent* file) {
    bld_string str = new_string();
    append_string(&str, file->d_name);
    return str;
}

bld_file_identifier get_identifier(bld_dirent* file) {
    return (bld_file_identifier) {.id = file->d_ino};
}

int file_eq(bld_file* f1, bld_file* f2) {
    return f1->identifier.id == f2->identifier.id;
}

bld_file make_file(bld_file_type type, bld_path* path, bld_dirent* file) {
    return (bld_file) {
        .type = type,
        .identifier = get_identifier(file),
        .name = extract_name(file),
        .path = *path,
        .compiler = NULL,
    };
}

bld_file make_header(bld_path* path, bld_dirent* file) {
    bld_file header = make_file(BLD_HEADER, path, file);
    return header;
}

bld_file make_impl(bld_path* path, bld_dirent* file) {
    bld_file impl = make_file(BLD_IMPL, path, file);
    return impl;
}

bld_file make_test(bld_path* path, bld_dirent* file) {
    bld_file test = make_file(BLD_TEST, path, file);
    return test;
}

void free_file(bld_file* file) {
    free_path(&file->path);
    free_string(&file->name);
    free_compiler(file->compiler);
}

bld_files new_files() {
    return (bld_files) {
        .capacity = 0,
        .size = 0,
        .files = NULL,
    };
}

void clear_files(bld_files* files) {
    files->size = 0;
}

void free_files(bld_files* files) {
    for (size_t i = 0; i < files->size; i++) {
        free_file(&files->files[i]);
    }
    free(files->files);
}

int append_file(bld_files* files, bld_file file) {
    bld_file* fs;
    size_t capacity = files->capacity;

    for (size_t i = 0; i < files->size; i++) {
        if (file_eq(&files->files[i], &file)) {
            return 1;
        }
    }


    if (files->size >= files->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);
        
        fs = malloc(capacity * sizeof(bld_file));
        if (fs == NULL) {
            log_fatal("Could not increase size of file array.");
        }

        memcpy(fs, files->files, files->size * sizeof(bld_file));
        free(files->files);
        
        files->capacity = capacity;
        files->files = fs;
    }

    files->files[files->size++] = file;
    return 0;
}
