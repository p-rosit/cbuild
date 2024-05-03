#ifndef APPLICATION_UTILS_H
#define APPLICATION_UTILS_H

#include "../bit_core/set.h"
#include "../bit_core/path.h"

extern const bld_string bld_path_build;
extern const bld_string bld_path_target;

typedef struct bld_data {
    bld_path root;
    bld_set targets;
} bld_data;

bld_data    data_extract(void);
void        data_free(bld_data*);

#endif
