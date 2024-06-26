#ifndef STRING_H
#define STRING_H

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#define STRING_COMPILE_TIME_PACK(str) {sizeof(str), sizeof(str) - 1, str}

typedef struct bld_string {
    size_t capacity;
    size_t size;
    char* chars;
} bld_string;

bld_string  string_new(void);
bld_string  string_pack(char*);
bld_string  string_copy(const bld_string*);
char*       string_unpack(const bld_string*);
void        string_free(bld_string*);
uintmax_t   string_hash(char*);
int         string_eq(const bld_string*, const bld_string*);

void        string_append_space(bld_string*);
void        string_append_char(bld_string*, char);
void        string_append_string(bld_string*, char*);

int         string_parse(FILE*, bld_string*);

#endif
