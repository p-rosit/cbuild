#include <string.h>
#include "logging.h"
#include "path.h"

bld_path path_new(void) {
    bld_path path;
    path.str = string_new();
    return path;
}

bld_path path_copy(bld_path* path) {
    bld_path cpy;
    cpy.str = string_copy(&path->str);
    return cpy;
}

void path_free(bld_path* path) {
    string_free(&path->str);
}

int path_eq(const bld_path* path1, const bld_path* path2) {
    return string_eq(&path1->str, &path2->str);
}

bld_path path_from_string(char* str) {
    bld_path path;
    path = path_new();
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
    size_t i, sep_len;
    char* str;

    sep_len = sizeof(BLD_PATH_SEP) - 1;
    for (i = 0; i < path->str.size; i++) {
        str = path->str.chars + path->str.size - i - 1;
        if (strncmp(str, BLD_PATH_SEP, sep_len) == 0) {
            return &path->str.chars[path->str.size - i + sizeof(BLD_PATH_SEP) - 2];
        }
    }

    return path->str.chars;
}

char* path_remove_last_string(bld_path* path) {
    char* name;
    name = path_get_last_string(path);
    if (name != path->str.chars) {
        path->str.size = name - path->str.chars - sizeof(BLD_PATH_SEP) + 1;
    } else {
        string_append_char(&path->str, '\0');
        memmove(path->str.chars + 1, path->str.chars, path->str.size - 1);

        name = path->str.chars + 1;
        path->str.size = 0;
    }

    path->str.chars[path->str.size] = '\0';
    return name;
}

int path_ends_with(bld_path* path, bld_path* suffix) {
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

void path_remove_file_ending(bld_path* path) {
    size_t i, sep_len;

    sep_len = sizeof(BLD_PATH_SEP) - 1;
    for (i = path->str.size; 0 < i; i--) {
        if (strncmp(&path->str.chars[i - 1], BLD_PATH_SEP, sep_len) == 0) {
            return;
        }
        if (path->str.chars[i - 1] == '.') {
            path->str.size = i - 1;
            path->str.chars[i - 1] = '\0';
            return;
        }
    }
}
