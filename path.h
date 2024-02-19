#ifndef PATH_H
#define PATH_H

#include "dstr.h"

typedef struct path {
    string str;
} path;

path new_path();
path copy_path(path*);

#endif
