#ifndef CONFIG_H
#define CONFIG_H
#include "../bld_core/logging.h"
#include "../bld_core/path.h"

typedef struct bld_config {
    bld_log_level log_level;
    int text_editor_configured;
    bld_string text_editor;
    int active_target_configured;
    bld_string active_target;
} bld_config;

bld_config config_new(void);
void config_free(bld_config*);
void serialize_config(bld_path*, bld_config*);
int parse_config(bld_path*, bld_config*);

#endif
