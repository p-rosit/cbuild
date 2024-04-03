#include <stdarg.h>
#include "dstr.h"
#include "linker.h"
#include "logging.h"
#include "json.h"

bld_linker linker_new(char* executable) {
    bld_linker linker;
    bld_string str = string_new();

    string_append_string(&str, executable);
    linker.executable = string_unpack(&str);
    linker.flags = linker_flags_new();
    
    return linker;
}

bld_linker linker_with_flags(char* executable, ...) {
    bld_linker linker;
    va_list args;
    char* flag;

    linker = linker_new(executable);

    va_start(args, executable);
    while (1) {
        flag = va_arg(args, char*);
        if (flag == NULL) {break;}

        linker_add_flag(&linker, flag);
    }

    return linker;
}

void linker_free(bld_linker* linker) {
    free(linker->executable);
    linker_flags_free(&linker->flags);
}

bld_linker linker_copy(bld_linker* linker) {
    bld_linker cpy;
    bld_string executable = string_new();
    string_append_string(&executable, linker->executable);

    cpy.executable = string_unpack(&executable);
    cpy.flags = linker_flags_copy(&linker->flags);

    return cpy;
}

void linker_add_flag(bld_linker* linker, char* flag) {
    linker_flags_add_flag(&linker->flags, flag);
}

bld_linker_flags linker_flags_new(void) {
    bld_linker_flags flags;

    flags.flag = array_new(sizeof(char*));
    flags.hash = array_new(sizeof(uintmax_t));

    return flags;
}

bld_linker_flags linker_flags_with_flags(char* first_flag, ...) {
    bld_linker_flags flags;
    va_list args;
    char* flag;

    flags = linker_flags_new();

    if (first_flag == NULL) {
        return flags;
    }
    linker_flags_add_flag(&flags, first_flag);

    va_start(args, first_flag);
    while (1) {
        flag = va_arg(args, char*);
        if (flag == NULL) {break;}

        linker_flags_add_flag(&flags, flag);
    }

    return flags;
}

void linker_flags_free(bld_linker_flags* flags) {
    bld_iter iter;
    char** flag;

    iter = iter_array(&flags->flag);
    while (iter_next(&iter, (void**) &flag)) {
        free(*flag);
    }
    array_free(&flags->flag);
    array_free(&flags->hash);
}

bld_linker_flags linker_flags_copy(bld_linker_flags* flags) {
    bld_string str;
    char **flag, *temp;
    bld_iter iter;
    bld_linker_flags cpy;

    cpy.flag = array_copy(&flags->flag);
    cpy.hash = array_copy(&flags->hash);

    iter = iter_array(&cpy.flag);
    while (iter_next(&iter, (void**) &flag)) {
        str = string_new();
        string_append_string(&str, *flag);
        temp = string_unpack(&str);

        *flag = temp;
    }

    return cpy;
}

void linker_flags_add_flag(bld_linker_flags* linker, char* flag) {
    uintmax_t flag_hash;
    char* temp;
    bld_string str = string_new();

    string_append_string(&str, flag);
    temp = string_unpack(&str);

    flag_hash = string_hash(temp, 76502);

    array_push(&linker->flag, &temp);
    array_push(&linker->hash, &flag_hash);
}

int parse_linker(FILE* file, bld_linker* linker) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"executable", "linker_flags"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_linker_executable,
        (bld_parse_func) parse_linker_linker_flags,
    };

    linker->flags = linker_flags_new();
    amount_parsed = parse_map(file, linker, size, parsed, (char**) keys, funcs);

    if (amount_parsed < size && !parsed[0]) {
        return -1;
    }
    return 0;
}

int parse_linker_executable(FILE* file, bld_linker* linker) {
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);

    if (result) {
        string_free(&str);
        log_warn("Could not parse linker executable");
        return -1;
    }

    temp = string_unpack(&str);
    linker->executable = temp;
    return result;
}

int parse_linker_linker_flags(FILE* file, bld_linker* linker) {
    bld_linker_flags flags = linker_flags_new();
    int result = parse_linker_flags(file, &flags);
    if (result) {
        linker_flags_free(&flags);
        log_warn("Could not parse linker flags");
    }

    linker->flags = flags;
    return result;
}

int parse_linker_flags(FILE* file, bld_linker_flags* flags) {
    int values;

    values = json_parse_array(file, flags, (bld_parse_func) parse_linker_flag);
    if (values < 0) {
        log_warn("Could not parse linker flags");
        return -1;
    }

    return 0;
}

int parse_linker_flag(FILE* file, bld_linker_flags* flags) {
    bld_string str = string_new();
    char* temp;
    uintmax_t temp_hash;
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse linker flag");
        return -1;
    }

    temp = string_unpack(&str);
    temp_hash = string_hash(temp, 76502);  /* Magic number from linker_flags_add_flag */
    array_push(&flags->flag, &temp);
    array_push(&flags->hash, &temp_hash);

    return result;
}