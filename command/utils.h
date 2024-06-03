#ifndef COMMAND_UTILS_H
#define COMMAND_UTILS_H

#include "../bld_core/set.h"
#include "../bld_core/path.h"
#include "../config/config.h"
#include "../config/config_target.h"
#include "handle.h"

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
    bld_set handles;
    bld_array handle_order;
} bld_data;

typedef int (bld_command_convert)(bld_command*, bld_data*, void*, void*);
typedef int (bld_command_execute)(void*, bld_data*);
typedef void (bld_command_free)(void*);

typedef enum bld_command_type {
    BLD_COMMAND_INVALID,
    BLD_COMMAND_INIT,
    BLD_COMMAND_SWITCH,
    BLD_COMMAND_REMOVE,
    BLD_COMMAND_HELP,
    BLD_COMMAND_ADD,
    BLD_COMMAND_BUILD,
    BLD_COMMAND_COMPILER,
    BLD_COMMAND_IGNORE,
    BLD_COMMAND_INVALIDATE,
    BLD_COMMAND_LINKER,
    BLD_COMMAND_STATUS
} bld_command_type;

typedef struct bld_handle_annotated {
    bld_command_type type;
    bld_handle handle;
    bld_command_convert* convert;
    bld_command_execute* execute;
    bld_command_free* free;
} bld_handle_annotated;

void        config_load(bld_data*);
void        config_save(bld_data*);
void        config_target_load(bld_data*, bld_string*);
void        config_target_save(bld_data*, bld_string*);
bld_data    data_extract(char*);
void        data_free(bld_data*);
int         utils_get_target(bld_string*, bld_string*, bld_command_positional_optional*, bld_data*);

bld_target_build_information* utils_get_build_info_for(bld_data*, bld_path*);
void utils_apply_build_information(bld_data*, bld_target_build_information*);

bld_target_build_information utils_index_project(bld_data*);

#endif
