#ifndef STRING_H
#define STRING_H

#include <stdlib.h>

typedef struct string {
    size_t capacity;
    size_t size;
    char* chars;
} string;

string new_string();
string copy_string(string*);
char* make_string(string*);
void free_string(string*);

void append_space(string*);
void append_string(string*, char*);

#endif
