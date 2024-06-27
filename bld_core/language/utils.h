#ifndef LANGUAGE_UTILS_H
#define LANGUAGE_UTILS_H
#include <stdio.h>

int get_next(FILE*);
int skip_line(FILE*);
int expect_char(FILE*, char);
int expect_string(FILE*, char*);

#endif
