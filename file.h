#ifndef FILE_H
#define FILE_H

#include "path.h"

typedef enum bld_file_type {
    INVALID,
    IMPL,
    HEADER,
    TEST,
} bld_file_type;

typedef struct bld_file {
    bld_file_type type;
    bld_path path;
} bld_file;

#endif
