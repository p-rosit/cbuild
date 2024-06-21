#ifndef PATH_H
#define PATH_H

#include "dstr.h"

#define BLD_PATH_SEP "/"

typedef struct bld_path {
    bld_string str;
} bld_path;

bld_path    path_new(void);
bld_path    path_copy(bld_path*);
void        path_free(bld_path*);
int         path_eq(const bld_path*, const bld_path*);

bld_path    path_from_string(char*);
char*       path_to_string(bld_path*);

void        path_append_string(bld_path*, char*);
void        path_append_path(bld_path*, bld_path*);
char*       path_get_last_string(bld_path*);
char*       path_remove_last_string(bld_path*);
void        path_remove_file_ending(bld_path*);
int         path_ends_with(bld_path*, bld_path*);

#endif
