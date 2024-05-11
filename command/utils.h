#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H

#include "../bld_core/set.h"
#include "../bld_core/path.h"
#include "../config/config.h"
#include "../config/config_target.h"

extern const bld_string bld_path_build;
extern const bld_string bld_path_target;

typedef struct bld_data {
    int has_root;
    bld_path root;
    bld_set targets;
    int config_parsed;
    bld_config config;
    int target_config_parsed;
    bld_config_target target_config;
} bld_data;

int         config_load(bld_data*, bld_config*);
void        config_save(bld_data*, bld_config*);
int         config_target_load(bld_data*, bld_string*, bld_config_target*);
void        config_target_save(bld_data*, bld_string*, bld_config_target*);
bld_data    data_extract(void);
void        data_free(bld_data*);

#endif
