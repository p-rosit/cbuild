#ifndef PATH_H
#define PATH_H

#include "dstr.h"

typedef struct bld_path {
    bld_string str;
} bld_path;

typedef struct bld_paths {
    size_t capacity;
    size_t size;
    bld_path* paths;
} bld_paths;

bld_path new_path();
bld_path copy_path(bld_path*);
void free_path(bld_path*);

bld_paths   new_paths();
void        free_paths(bld_paths*);
void        push_path(bld_paths*, bld_path);

bld_path    path_from_string(char*);
char*       path_to_string(bld_path*);

void        append_dir(bld_path*, char*);
void        append_path(bld_path*, bld_path*);
char*       remove_last_dir(bld_path*);
void        remove_file_ending(bld_path*);
int         path_ends_with(bld_path*, bld_path*);

#endif
