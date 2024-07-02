#ifndef COMMAND_TEST_H
#define COMMAND_TEST_H
#include "../bld_core/dstr.h"
#include "../bld_core/args.h"
#include "handle.h"
#include "invalid.h"

extern const bld_string bld_command_string_test;

typedef struct bld_command_test {
    bld_string target;
    bld_path test_path;
} bld_command_test;

bld_handle_annotated command_handle_test(char*);
int command_test_convert(bld_command*, bld_data*, bld_command_test*, bld_command_invalid*);
int command_test(bld_command_test*, bld_data*);
void command_test_free(bld_command_test*);

#endif
