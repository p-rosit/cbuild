#ifndef CONFIG_H
#define CONFIG_H
#include "../bld_core/path.h"

typedef struct bld_config {
    int text_editor_configured;
    bld_string text_editor;
    int default_target_configured;
    bld_string target;
} bld_config;

bld_config config_new(void);
void config_free(bld_config*);
void serialize_config(bld_path*, bld_config*);
int parse_config(bld_path*, bld_config*);

#endif
