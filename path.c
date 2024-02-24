#include <string.h>
#include "build.h"
#include "path.h"

#define BLD_PATH_SEP ("/")

bld_paths new_paths() {
    return (bld_paths) {
        .capacity = 0,
        .size = 0,
        .paths = NULL,
    };
}

void free_paths(bld_paths* paths) {
    for (size_t i = 0; i < paths->size; i++) {
        free_path(&paths->paths[i]);
    }
    free(paths->paths);
}

void push_path(bld_paths* paths, bld_path path) {
    size_t capacity = paths->capacity;
    bld_path* temp;

    if (paths->size >= paths->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        temp = malloc(capacity * sizeof(bld_path));
        if (temp == NULL) {log_fatal("Could not append path \"%s\"", path_to_string(&path));}

        memcpy(temp, paths->paths, paths->size * sizeof(bld_path));
        free(paths->paths);

        paths->capacity = capacity;
        paths->paths = temp;
    }

    paths->paths[paths->size++] = path;
}

bld_path new_path() {
    return (bld_path) {
        .str = new_string(),
    };
}

bld_path copy_path(bld_path* path) {
    return (bld_path) {
        .str = copy_string(&path->str),
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
    return make_string(&path->str);
}

void append_dir(bld_path* path, char* str) {
    append_string(&path->str, BLD_PATH_SEP);
    append_string(&path->str, str);
}

void append_path(bld_path* root, bld_path* path) {
    append_dir(root, make_string(&path->str));
}

void remove_last_dir(bld_path* path) {
    size_t sep_len = sizeof(BLD_PATH_SEP) - 1;
    char* str;

    for (size_t i = 0; i < path->str.size; i++) {
        str = path->str.chars + path->str.size - i - 1;
        if (strncmp(str, BLD_PATH_SEP, sep_len) == 0) {
            path->str.size -= i + 1;
            return;
        }
    }

    log_fatal("remove_last_dir: directory contains only one name...");
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
    size_t i;

    for (i = path->str.size; 0 < i; i--) {
        if (path->str.chars[i - 1] == '.') {
            path->str.size = i - 1;
            path->str.chars[i - 1] = '\0';
            return;
        }
    }
}
