#ifndef STRING_H
#define STRING_H

#include <stdlib.h>

typedef struct bld_string {
    size_t capacity;
    size_t size;
    char* chars;
} bld_string;

bld_string new_string();
bld_string copy_string(bld_string*);
char* make_string(bld_string*);
void free_string(bld_string*);

void append_space(bld_string*);
void append_string(bld_string*, char*);

#endif
