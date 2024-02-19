#ifndef PATH_H
#define PATH_H

#include "dstr.h"

typedef struct path {
    string str;
} path;

path new_path();
path copy_path(path*);
void free_path(path*);

path path_from_string(char*);
char* path_to_string(path*);

void append_dir(path*, char*);
void append_path(path*, path*);

#endif
