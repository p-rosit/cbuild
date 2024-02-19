#include "path.h"

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

