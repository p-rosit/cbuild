#ifndef COMMAND_PARSE_H
#define COMMAND_PARSE_H
#include "../bld_core/dstr.h"
#include "../bld_core/array.h"
#include "../bld_core/set.h"
#include "../bld_core/args.h"

#define BLD_COMMAND_MAX_ARGS (100)
#define BLD_COMMAND_MAX_ARGS_LEN (3)

typedef enum bld_handle_positional_type {
    BLD_HANDLE_POSITIONAL_REQUIRED,
    BLD_HANDLE_POSITIONAL_OPTIONAL,
    BLD_HANDLE_POSITIONAL_VARGS,
    BLD_HANDLE_POSITIONAL_EXPECTED
} bld_handle_positional_type;

typedef struct bld_handle_positional {
    bld_handle_positional_type type;
    bld_string description;
} bld_handle_positional;

typedef struct bld_handle_flag {
    char swtch;
    bld_string option;
    bld_string description;
} bld_handle_flag;

typedef struct bld_handle {
    bld_string name;
    int description_set;
    bld_string description;
    bld_array positional;
    int arbitrary_flags;
    bld_string arbitray_flags_description;
    int flag_start_index;
    bld_array flag_array;
    bld_set flags;
} bld_handle;

bld_handle  handle_new(char*);
void        handle_free(bld_handle*);
bld_string  handle_make_description(bld_handle*);

void handle_positional_required(bld_handle*, char*);
void handle_positional_vargs(bld_handle*, char*);
void handle_positional_optional(bld_handle*, char*);
void handle_positional_expect(bld_handle*, char*);

void handle_allow_arbitrary_flags(bld_handle*, char*);
void handle_allow_flags(bld_handle*);

void handle_flag(bld_handle*, char, char*, char*);
void handle_set_description(bld_handle*, char*);

#endif
