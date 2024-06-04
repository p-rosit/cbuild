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

typedef struct bld_command_positional_required {
    bld_string value;
} bld_command_positional_required;

typedef struct bld_command_positional_optional {
    int present;
    bld_string value;
} bld_command_positional_optional;

typedef struct bld_command_positional_vargs {
    bld_array values;
} bld_command_positional_vargs;

typedef struct bld_command_positional_expected {
    bld_string value;
} bld_command_positional_expected;

typedef union bld_command_positional_union {
    bld_command_positional_required req;
    bld_command_positional_optional opt;
    bld_command_positional_vargs vargs;
    bld_command_positional_expected exp;
} bld_command_positional_union;

typedef struct bld_command_positional {
    bld_handle_positional_type type;
    bld_command_positional_union as;
} bld_command_positional;

typedef struct bld_command_flag {
    int is_switch;
    bld_string flag;
} bld_command_flag;

typedef enum bld_command_error {
    BLD_COMMAND_ERROR_ARGS_TOO_FEW = (1 << 0),
    BLD_COMMAND_ERROR_ARGS_TOO_MANY = (1 << 1),
    BLD_COMMAND_ERROR_ARGS_NO_MATCH = (1 << 2),
    BLD_COMMAND_ERROR_FLAG_UNKNOWN = (1 << 3),
    BLD_COMMAND_ERROR_FLAG_EMPTY = (1 << 4),
    BLD_COMMAND_ERROR_FLAG_DUPLICATE = (1 << 5),
    BLD_COMMAND_ERROR_FLAG_EARLY = (1 << 6)
} bld_command_error;

typedef struct bld_command {
    bld_array positional;
    bld_set flags;
    bld_array extra_flags;
} bld_command;

bld_handle  handle_new(char*);
void        handle_free(bld_handle*);
bld_string  handle_make_description(bld_handle*);
int         handle_parse(bld_args, bld_handle*, bld_command*, bld_array*);
void        command_free(bld_command*);

void handle_positional_required(bld_handle*, char*);
void handle_positional_vargs(bld_handle*, char*);
void handle_positional_optional(bld_handle*, char*);
void handle_positional_expect(bld_handle*, char*);

void handle_allow_arbitrary_flags(bld_handle*, char*);
void handle_allow_flags(bld_handle*);

void handle_flag(bld_handle*, char, char*, char*);
void handle_set_description(bld_handle*, char*);

#endif
