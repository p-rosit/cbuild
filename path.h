#ifndef PATH_H
#define PATH_H

#include "dstr.h"

#define BIT_PATH_SEP "/"

typedef struct bit_path {
    bit_string str;
} bit_path;

bit_path    path_new(void);
bit_path    path_copy(bit_path*);
void        path_free(bit_path*);

bit_path    path_from_string(char*);
char*       path_to_string(bit_path*);

void        path_append_string(bit_path*, char*);
void        path_append_path(bit_path*, bit_path*);
char*       path_get_last_string(bit_path*);
char*       path_remove_last_string(bit_path*);
void        path_remove_file_ending(bit_path*);
int         path_ends_with(bit_path*, bit_path*);

#endif
