#ifndef CONFIG_TARGET_H
#define CONFIG_TARGET_H
#include "../bld_core/array.h"
#include "../bld_core/path.h"
#include "../bld_core/file.h"

typedef struct bld_target_build_information {
    bld_string name;
    bld_file_build_information info;
    bld_array files;
} bld_target_build_information;

typedef struct bld_config_target {
    bld_path path_main;
    bld_array added_paths;
    bld_array ignore_paths;
    int linker_set;
    bld_linker linker;
    int files_set;
    bld_target_build_information files;
} bld_config_target;

bld_config_target config_target_new(bld_path*);
void config_target_free(bld_config_target*);
void serialize_config_target(bld_path*, bld_config_target*);
int parse_config_target(bld_path*, bld_config_target*);

#endif
