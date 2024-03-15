#ifndef STRING_H
#define STRING_H

#include <inttypes.h>
#include <stdlib.h>

typedef struct bld_string {
    size_t capacity;
    size_t size;
    char* chars;
} bld_string;

bld_string  string_new();
bld_string  copy_string(bld_string*);
char*       make_string(bld_string*);
void        free_string(bld_string*);
uintmax_t   hash_string(char*, uintmax_t);

void        append_space(bld_string*);
void        append_char(bld_string*, char);
void        append_string(bld_string*, char*);

#endif
