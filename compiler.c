#include <string.h>
#include <stdarg.h>
#include "logging.h"
#include "json.h"
#include "compiler.h"

bld_compiler compiler_new(char* executable) {
    bld_compiler compiler;
    bld_string str = string_new();

    string_append_string(&str, executable);

    compiler.executable = string_unpack(&str);
    compiler.flags = array_new(sizeof(char*));

    return compiler;
}

bld_compiler compiler_with_flags(char* executable, ...) {
    bld_compiler compiler;
    va_list args;
    char* flag;

    compiler = compiler_new(executable);

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
    bld_iter iter;

    if (compiler == NULL) {return;}
    
    free(compiler->executable);

    iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        free(*flag);
    }
    array_free(&compiler->flags);
}

bld_compiler compiler_copy(bld_compiler* compiler) {
    bld_string str;
    char **flag, *temp;
    bld_compiler cpy;
    bld_iter iter;
    bld_array flags;
    bld_string executable = string_new();
    string_append_string(&executable, compiler->executable);

    flags = array_copy(&compiler->flags);

    iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        str = string_new();
        string_append_string(&str, *flag);
        temp = string_unpack(&str);

        *flag = temp;
    }

    cpy.executable = string_unpack(&executable);
    cpy.flags = flags;

    return cpy;
}

uintmax_t compiler_hash(bld_compiler* compiler, uintmax_t seed) {
    char** flag;
    bld_iter iter;

    seed = string_hash(compiler->executable, seed);
    
    iter = iter_array(&compiler->flags);
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
}

int parse_compiler(FILE* file, bld_compiler* compiler) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"executable", "flags"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_compiler_executable,
        (bld_parse_func) parse_compiler_flags,
    };

    compiler->flags = array_new(sizeof(char*));
    amount_parsed = json_parse_map(file, compiler, size, parsed, (char**) keys, funcs);

    if (amount_parsed < size && !parsed[0]) {
        return -1;
    }
    return 0;
}

int parse_compiler_executable(FILE* file, bld_compiler* compiler) {
    bld_string str = string_new();
    char* temp;
    int result = string_parse(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler executable");
        return -1;
    }

    temp = string_unpack(&str);
    compiler->executable = temp;
    return result;
}

int parse_compiler_flags(FILE* file, bld_compiler* compiler) {
    int values;
    bld_iter iter;
    char** flag;
    bld_array flags = array_new(sizeof(char*));

    values = json_parse_array(file, &flags, (bld_parse_func) parse_compiler_option);
    if (values < 0) {
        iter = iter_array(&flags);
        while (iter_next(&iter, (void**) &flag)) {
            free(*flag);
        }
        array_free(&flags);

        log_warn("Could not parse compiler flags");
        return -1;
    }

    compiler->flags = flags;
    return 0;
}

int parse_compiler_option(FILE* file, bld_array* flags) {
    bld_string str = string_new();
    char* temp;
    int result = string_parse(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler flag");
        return -1;
    }

    temp = string_unpack(&str);
    array_push(flags, &temp);

    return result;
}