#ifndef STRING_H
#define STRING_H

#include <inttypes.h>
#include <stdlib.h>

typedef struct bld_string {
    size_t capacity;
    size_t size;
    char* chars;
} bld_string;

bld_string  string_new(void);
bld_string  string_copy(bld_string*);
char*       string_unpack(bld_string*);
void        string_free(bld_string*);
uintmax_t   string_hash(char*, uintmax_t);

void        string_append_space(bld_string*);
void        string_append_char(bld_string*, char);
void        string_append_string(bld_string*, char*);

#endif
