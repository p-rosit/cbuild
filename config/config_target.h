#ifndef CONFIG_TARGET_H
#define CONFIG_TARGET_H
#include "../bld_core/path.h"

typedef struct bld_config_target {
    bld_path path_main;
} bld_config_target;

bld_config_target config_target_new(bld_path*);
void config_target_free(bld_config_target*);
void serialize_config_target(bld_path*, bld_config_target*);
int parse_config_target(bld_path*, bld_config_target*);

#endif
