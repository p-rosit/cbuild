#include <string.h>
#include <stdarg.h>
#include "logging.h"
#include "compiler.h"

bld_compiler compiler_new(bld_compiler_type type, char* executable) {
    bld_string str = string_new();
    string_append_string(&str, executable);

    return (bld_compiler) {
        .type = type,
        .executable = string_unpack(&str),
        .flags = array_new(sizeof(char*)),
    };
}

bld_compiler compiler_with_flags(bld_compiler_type type, char* executable, ...) {
    bld_compiler compiler;
    va_list args;
    char* flag;

    compiler = compiler_new(type, executable);

    va_start(args, executable);
    while (1) {
        flag = va_arg(args, char*);
        if (flag == NULL) {break;}

        compiler_add_flag(&compiler, flag);
    }

    return compiler;
}

void compiler_free(bld_compiler* compiler) {
    char** flag;

    if (compiler == NULL) {return;}
    
    free(compiler->executable);

    bld_iter iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        free(*flag);
    }
    array_free(&compiler->flags);
}

bld_compiler compiler_copy(bld_compiler* compiler) {
    bld_string str;
    char **flag, *temp;
    bld_array flags;
    bld_string executable = string_new();
    string_append_string(&executable, compiler->executable);

    flags = array_copy(&compiler->flags);

    bld_iter iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        str = string_new();
        string_append_string(&str, *flag);
        temp = string_unpack(&str);

        *flag = temp;
    }

    return (bld_compiler) {
        .type = compiler->type,
        .executable = string_unpack(&executable),
        .flags = flags,
    };
}

uintmax_t compiler_hash(bld_compiler* compiler, uintmax_t seed) {
    char** flag;

    seed = string_hash(compiler->executable, seed);
    
    bld_iter iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        seed = string_hash(*flag, seed);
    }

    return seed;
}

void compiler_add_flag(bld_compiler* compiler, char* flag) {
    char* temp;
    bld_string s = string_new();

    string_append_string(&s, flag);
    temp = string_unpack(&s);
    array_push(&compiler->flags, &temp);

    log_debug("Added flag: \"%s\"", flag);
}
