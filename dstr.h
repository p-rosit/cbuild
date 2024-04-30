#ifndef STRING_H
#define STRING_H

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct bit_string {
    size_t capacity;
    size_t size;
    char* chars;
} bit_string;

bit_string  string_new(void);
bit_string  string_pack(char*);
bit_string  string_copy(bit_string*);
char*       string_unpack(bit_string*);
void        string_free(bit_string*);
uintmax_t   string_hash(char*);

void        string_append_space(bit_string*);
void        string_append_char(bit_string*, char);
void        string_append_string(bit_string*, char*);

int         string_parse(FILE*, bit_string*);

#endif
