#ifndef JSON_H
#include <stdio.h>
#include <inttypes.h>
#include "iter.h"

typedef void (*bit_serialize_func)(FILE*, void*);
typedef int (*bit_parse_func)(FILE*, void*);

void json_serialize_array(FILE*, bit_iter, bit_serialize_func);
void json_serialize_map(FILE*, void*, int, char**, bit_serialize_func*);

int json_parse_array(FILE*, void*, bit_parse_func);
int json_parse_map(FILE*, void*, int, int*, char**, bit_parse_func*);

int parse_uintmax(FILE*, uintmax_t*);
int next_character(FILE*);

#endif
