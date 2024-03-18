#ifndef PATH_H
#define PATH_H

#include "container.h"
#include "dstr.h"

#define BLD_PATH_SEP "/"

typedef struct bld_path {
    bld_string str;
} bld_path;

typedef struct bld_paths {
    bld_array array;
} bld_paths;

bld_path    path_new();
bld_path    path_copy(bld_path*);
void        free_path(bld_path*);

bld_paths   new_paths();
void        free_paths(bld_paths*);
void        push_path(bld_paths*, bld_path);

bld_path    path_from_string(char*);
char*       path_to_string(bld_path*);

void        append_dir(bld_path*, char*);
void        append_path(bld_path*, bld_path*);
char*       get_last_dir(bld_path*);
char*       remove_last_dir(bld_path*);
void        remove_file_ending(bld_path*);
int         path_ends_with(bld_path*, bld_path*);

#endif
