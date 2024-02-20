#ifndef PATH_H
#define PATH_H

#include "dstr.h"

typedef struct bld_path {
    bld_string str;
} bld_path;

bld_path new_path();
bld_path copy_path(bld_path*);
void free_path(bld_path*);

bld_path path_from_string(char*);
char* path_to_string(bld_path*);

void append_dir(bld_path*, char*);
void append_path(bld_path*, bld_path*);
void remove_last_dir(bld_path*);

#endif
