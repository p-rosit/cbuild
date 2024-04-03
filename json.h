#ifndef JSON_H
#include <stdio.h>
#include "container.h"

typedef void (*bld_serialize_func)(FILE*, void*);
typedef int (*bld_parse_func)(FILE*, void*);

void json_serialize_array(FILE*, bld_iter, bld_serialize_func);
void json_serialize_map(FILE*, void*, int, char**, bld_serialize_func*);

int json_parse_array(FILE*, void*, bld_parse_func);
int json_parse_map(FILE*, void*, int, int*, char**, bld_parse_func*);

int parse_uintmax(FILE*, uintmax_t*);
int next_character(FILE*);

#endif