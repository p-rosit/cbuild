#ifndef COMMAND_INIT_H
#define COMMAND_INIT_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "handle.h"
#include "invalid.h"

extern const bld_string bld_command_string_init;
extern const bld_string bld_command_init_missing_project;
extern const bld_string bld_command_init_no_targets;

typedef struct bld_command_init {
    int init_project;
    bld_string target;
    bld_path path_main;
} bld_command_init;

bld_handle_annotated command_handle_init(char*);
int command_init_convert(bld_command*, bld_data*, bld_command_init*, bld_command_invalid*);
int command_init(bld_command_init*, bld_data*);
void command_init_free(bld_command_init*);

#endif
