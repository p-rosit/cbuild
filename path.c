#include <string.h>
#include "logging.h"
#include "path.h"

bit_path path_new(void) {
    bit_path path;
    path.str = string_new();
    return path;
}

bit_path path_copy(bit_path* path) {
    bit_path cpy;
    cpy.str = string_copy(&path->str);
    return cpy;
}

void path_free(bit_path* path) {
    string_free(&path->str);
}

bit_path path_from_string(char* str) {
    bit_path path = path_new();
    string_append_string(&path.str, str);
    return path;
}

char* path_to_string(bit_path* path) {
    return string_unpack(&path->str);
}

void path_append_string(bit_path* path, char* str) {
    string_append_string(&path->str, BIT_PATH_SEP);
    string_append_string(&path->str, str);
}

void path_append_path(bit_path* root, bit_path* path) {
    path_append_string(root, path_to_string(path));
}

char* path_get_last_string(bit_path* path) {
    size_t i, sep_len;
    char* str;

    sep_len = sizeof(BIT_PATH_SEP) - 1;
    for (i = 0; i < path->str.size; i++) {
        str = path->str.chars + path->str.size - i - 1;
        if (strncmp(str, BIT_PATH_SEP, sep_len) == 0) {
            return &path->str.chars[path->str.size - i + sizeof(BIT_PATH_SEP) - 2];
        }
    }

    log_fatal("path_get_last_string: directory contains only one name...");
    return NULL; /* Unreachable */
}

char* path_remove_last_string(bit_path* path) {
    char* name = path_get_last_string(path);
    path->str.size = name - path->str.chars - sizeof(BIT_PATH_SEP) + 1;
    path->str.chars[path->str.size] = '\0';
    return name;
}

int path_ends_with(bit_path* path, bit_path* suffix) {
    size_t i, suffix_start;
    char a, b;

    suffix_start = path->str.size - suffix->str.size;
    if (suffix->str.size > path->str.size) {return 0;}

    for (i = 0; i < suffix->str.size; i++) {
        a = path->str.chars[suffix_start + i];
        b = suffix->str.chars[i];
        if (a != b) {
            return 0;
        }
    }

    return 1;
}

void path_remove_file_ending(bit_path* path) {
    size_t i, sep_len;

    sep_len = sizeof(BIT_PATH_SEP) - 1;
    for (i = path->str.size; 0 < i; i--) {
        if (strncmp(path->str.chars, BIT_PATH_SEP, sep_len) == 0) {
            return;
        }
        if (path->str.chars[i - 1] == '.') {
            path->str.size = i - 1;
            path->str.chars[i - 1] = '\0';
            return;
        }
    }
}
