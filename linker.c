#include <stdarg.h>
#include "dstr.h"
#include "linker.h"
#include "logging.h"
#include "json.h"

bld_linker linker_new(char* executable) {
    bld_linker linker;
    bld_string str;

    str = string_pack(executable);
    linker.executable = string_copy(&str);
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
    string_free(&linker->executable);
    linker_flags_free(&linker->flags);
}

bld_linker linker_copy(bld_linker* linker) {
    bld_linker cpy;

    cpy.executable = string_copy(&linker->executable);
    cpy.flags = linker_flags_copy(&linker->flags);

    return cpy;
}

void linker_add_flag(bld_linker* linker, char* flag) {
    linker_flags_add_flag(&linker->flags, flag);
}

bld_linker_flags linker_flags_new(void) {
    bld_linker_flags flags;
    flags.flags = array_new(sizeof(bld_string));
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
    bld_string* flag;

    iter = iter_array(&flags->flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(flag);
    }
    array_free(&flags->flags);
}

bld_linker_flags linker_flags_copy(bld_linker_flags* flags) {
    bld_string str;
    bld_string *flag;
    bld_iter iter;
    bld_linker_flags cpy;

    cpy.flags = array_copy(&flags->flags);

    iter = iter_array(&cpy.flags);
    while (iter_next(&iter, (void**) &flag)) {
        str = string_copy(flag);
        *flag = str;
    }

    return cpy;
}

void linker_flags_add_flag(bld_linker_flags* linker, char* flag) {
    bld_string str;

    str = string_pack(flag);
    str = string_copy(&str);
    array_push(&linker->flags, &str);
}

void linker_flags_collect(bld_string* str, bld_array* linker_flags) {
    bld_set added;
    bld_iter iter;
    bld_linker_flags** flags;

    added = set_new(0);
    iter = iter_array(linker_flags);
    while (iter_next(&iter, (void**) &flags)) {
        bld_iter iter;
        bld_string* f;
        uintmax_t hash;

        iter = iter_array(&(*flags)->flags);
        while (iter_next(&iter, (void**) &f)) {
            hash = string_hash(string_unpack(f), 0);

            if (set_has(&added, hash)) {
                continue;
            }

            set_add(&added, hash, NULL);
            string_append_space(str);
            string_append_string(str, string_unpack(f));
        }
    }

    set_free(&added);
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
    amount_parsed = json_parse_map(file, linker, size, parsed, (char**) keys, funcs);

    if (amount_parsed < size && !parsed[0]) {
        return -1;
    }
    return 0;
}

int parse_linker_executable(FILE* file, bld_linker* linker) {
    bld_string str;
    int result = string_parse(file, &str);

    if (result) {
        log_warn("Could not parse linker executable");
        return -1;
    }

    linker->executable = str;
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
    bld_string str;
    int result = string_parse(file, &str);
    if (result) {
        log_warn("Could not parse linker flag");
        return -1;
    }

    array_push(&flags->flags, &str);

    return result;
}
