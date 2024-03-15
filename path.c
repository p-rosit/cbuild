#include <string.h>
#include "logging.h"
#include "path.h"

bld_paths new_paths() {
    return (bld_paths) {.array = bld_array_new(sizeof(bld_path))};
}

void free_paths(bld_paths* paths) {
    bld_path* ps = paths->array.values;
    for (size_t i = 0; i < paths->array.size; i++) {
        free_path(&ps[i]);
    }
    bld_array_free(&paths->array);
}

void push_path(bld_paths* paths, bld_path path) {
    bld_array_push(&paths->array, &path);
}

bld_path new_path() {
    return (bld_path) {
        .str = string_new(),
    };
}

bld_path copy_path(bld_path* path) {
    return (bld_path) {
        .str = string_copy(&path->str),
    };
}

void free_path(bld_path* path) {
    free_string(&path->str);
}

bld_path path_from_string(char* str) {
    bld_path path = new_path();
    append_string(&path.str, str);
    return path;
}

char* path_to_string(bld_path* path) {
    return string_unpack(&path->str);
}

void append_dir(bld_path* path, char* str) {
    append_string(&path->str, BLD_PATH_SEP);
    append_string(&path->str, str);
}

void append_path(bld_path* root, bld_path* path) {
    append_dir(root, string_unpack(&path->str));
}

char* get_last_dir(bld_path* path) {
    size_t sep_len = sizeof(BLD_PATH_SEP) - 1;
    char* str;

    path_to_string(path);
    for (size_t i = 0; i < path->str.size; i++) {
        str = path->str.chars + path->str.size - i - 1;
        if (strncmp(str, BLD_PATH_SEP, sep_len) == 0) {
            return &path->str.chars[path->str.size - i + sizeof(BLD_PATH_SEP) - 2];
        }
    }

    log_fatal("remove_last_dir: directory contains only one name...");
    return NULL; /* Unreachable */
}

char* remove_last_dir(bld_path* path) {
    char* name = get_last_dir(path);
    path->str.size = name - path->str.chars - sizeof(BLD_PATH_SEP) + 1;
    return name;
}

int path_ends_with(bld_path* path, bld_path* suffix) {
    size_t suffix_start = path->str.size - suffix->str.size;
    char a, b;

    if (suffix->str.size > path->str.size) {return 0;}

    for (size_t i = 0; i < suffix->str.size; i++) {
        a = path->str.chars[suffix_start + i];
        b = suffix->str.chars[i];
        if (a != b) {
            return 0;
        }
    }

    return 1;
}

void remove_file_ending(bld_path* path) {
    /* TODO: make sure we don't pass a path separator */
    for (size_t i = path->str.size; 0 < i; i--) {
        if (path->str.chars[i - 1] == '.') {
            path->str.size = i - 1;
            path->str.chars[i - 1] = '\0';
            return;
        }
    }
}
