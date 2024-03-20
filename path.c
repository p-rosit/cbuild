#include <string.h>
#include "logging.h"
#include "path.h"

bld_path path_new(void) {
    return (bld_path) {
        .str = string_new(),
    };
}

bld_path path_copy(bld_path* path) {
    return (bld_path) {
        .str = string_copy(&path->str),
    };
}

void path_free(bld_path* path) {
    string_free(&path->str);
}

bld_path path_from_string(char* str) {
    bld_path path = path_new();
    string_append_string(&path.str, str);
    return path;
}

char* path_to_string(bld_path* path) {
    return string_unpack(&path->str);
}

void path_append_string(bld_path* path, char* str) {
    string_append_string(&path->str, BLD_PATH_SEP);
    string_append_string(&path->str, str);
}

void path_append_path(bld_path* root, bld_path* path) {
    path_append_string(root, path_to_string(path));
}

char* path_get_last_string(bld_path* path) {
    size_t sep_len = sizeof(BLD_PATH_SEP) - 1;
    char* str;

    path_to_string(path);
    for (size_t i = 0; i < path->str.size; i++) {
        str = path->str.chars + path->str.size - i - 1;
        if (strncmp(str, BLD_PATH_SEP, sep_len) == 0) {
            return &path->str.chars[path->str.size - i + sizeof(BLD_PATH_SEP) - 2];
        }
    }

    log_fatal("path_remove_last_string: directory contains only one name...");
    return NULL; /* Unreachable */
}

char* path_remove_last_string(bld_path* path) {
    char* name = path_get_last_string(path);
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

void path_remove_file_ending(bld_path* path) {
    /* TODO: make sure we don't pass a path separator */
    for (size_t i = path->str.size; 0 < i; i--) {
        if (path->str.chars[i - 1] == '.') {
            path->str.size = i - 1;
            path->str.chars[i - 1] = '\0';
            return;
        }
    }
}
