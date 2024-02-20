#include <string.h>
#include "build.h"
#include "path.h"

#define BLD_PATH_SEP ("/")

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
    append_string(&path->str, str);
}

void append_path(bld_path* root, bld_path* path) {
    append_string(&root->str, make_string(&path->str));
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

