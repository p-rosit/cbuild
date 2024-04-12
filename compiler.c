#include <string.h>
#include <stdarg.h>
#include "logging.h"
#include "json.h"
#include "compiler.h"

bld_compiler compiler_new(char* executable) {
    bld_compiler compiler;
    bld_string str;

    str = string_pack(executable);
    compiler.executable = string_copy(&str);
    compiler.flags = array_new(sizeof(bld_string));

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
    bld_iter iter;
    bld_string* flag;

    if (compiler == NULL) {return;}
    
    string_free(&compiler->executable);

    iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(flag);
    }
    array_free(&compiler->flags);
}

bld_compiler compiler_copy(bld_compiler* compiler) {
    bld_string str, *flag;
    bld_compiler cpy;
    bld_iter iter;
    bld_array flags;

    flags = array_copy(&compiler->flags);

    iter = iter_array(&flags);
    while (iter_next(&iter, (void**) &flag)) {
        str = string_copy(flag);
        *flag = str;
    }

    cpy.executable = string_copy(&compiler->executable);
    cpy.flags = flags;

    return cpy;
}

uintmax_t compiler_hash(bld_compiler* compiler, uintmax_t seed) {
    bld_iter iter;
    bld_string* flag;

    seed = string_hash(string_unpack(&compiler->executable), seed);
    
    iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        seed = string_hash(string_unpack(flag), seed);
    }

    return seed;
}

void compiler_add_flag(bld_compiler* compiler, char* flag) {
    bld_string str = string_pack(flag);
    str = string_copy(&str);
    array_push(&compiler->flags, &str);
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

    compiler->flags = array_new(sizeof(bld_string));
    amount_parsed = json_parse_map(file, compiler, size, parsed, (char**) keys, funcs);

    if (amount_parsed < size && !parsed[0]) {
        return -1;
    }
    return 0;
}

int parse_compiler_executable(FILE* file, bld_compiler* compiler) {
    bld_string str;
    int result = string_parse(file, &str);
    if (result) {
        log_warn("Could not parse compiler executable");
        return -1;
    }

    compiler->executable = str;
    return result;
}

int parse_compiler_flags(FILE* file, bld_compiler* compiler) {
    int values;
    bld_iter iter;
    bld_string* flag;
    bld_array flags;

    flags = array_new(sizeof(bld_string));
    values = json_parse_array(file, &flags, (bld_parse_func) parse_compiler_option);
    if (values < 0) {
        iter = iter_array(&flags);
        while (iter_next(&iter, (void**) &flag)) {
            string_free(flag);
        }
        array_free(&flags);

        log_warn("Could not parse compiler flags");
        return -1;
    }

    compiler->flags = flags;
    return 0;
}

int parse_compiler_option(FILE* file, bld_array* flags) {
    bld_string str;
    int result = string_parse(file, &str);
    if (result) {
        log_warn("Could not parse compiler flag");
        return -1;
    }

    array_push(flags, &str);

    return result;
}
