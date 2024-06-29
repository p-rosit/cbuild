#include <string.h>
#include "../logging.h"
#include "../iter.h"
#include "compiler.h"
#include "gcc.h"
#include "clang.h"

int compiler_type_file_is_implementation(bld_compiler_type, bld_string*);
int compiler_type_file_is_header(bld_compiler_type, bld_string*);

bld_array compiler_get_available(void) {
    bld_array compilers = array_new(sizeof(bld_string));

    array_push(&compilers, &bld_compiler_string_gcc);
    array_push(&compilers, &bld_compiler_string_gpp);
    array_push(&compilers, &bld_compiler_string_clang);

    return compilers;
}

bld_compiler_type compiler_get_mapping(bld_string* name) {
    size_t i;
    bld_string* compilers[] = {
        &bld_compiler_string_gcc,
        &bld_compiler_string_gpp,
        &bld_compiler_string_clang
    };
    bld_compiler_type types[] = {
        BLD_COMPILER_GCC,
        BLD_COMPILER_GCC,
        BLD_COMPILER_CLANG
    };

    if (sizeof(compilers) / sizeof(*compilers) != sizeof(types) / sizeof(*types)) {
        log_fatal("compiler_get_mapping: internal error");
    }

    for (i = 0; i < sizeof(types) / sizeof(*types); i++) {
        if (string_eq(name, compilers[i])) {
            return types[i];
        }
    }

    return BLD_COMPILER_AMOUNT;
}

bld_string* compiler_get_string(bld_compiler_type type) {
    bld_string* compilers[] = {
        &bld_compiler_string_gcc,
        &bld_compiler_string_clang
    };

    if (sizeof(compilers) / sizeof(*compilers) != BLD_COMPILER_AMOUNT) {
        log_fatal("compiler_get_string: incorrect amount of strings");
    }

    if (type < 0 || type >= BLD_COMPILER_AMOUNT) {
        log_fatal("compiler_get_string: type %d cannot be converted to handle", type);
    }

    return compilers[type];
}

bld_string compiler_get_file_extension(bld_string* name) {
    char* temp;

    temp = strrchr(string_unpack(name), '.');
    if (temp == NULL) {
        temp = name->chars + name->size - 1;
    }

    return string_pack(temp + 1);
}

int compiler_file_is_implementation(bld_set* handles, bld_string* name) {
    int match;
    bld_iter iter;
    bld_compiler_type* type;

    match = 0;
    iter = iter_set(handles);
    while (iter_next(&iter, (void**) &type)) {
        match = match || compiler_type_file_is_implementation(*type, name);
    }

    return match;
}

int compiler_file_is_header(bld_set* handles, bld_string* name) {
    int match;
    bld_iter iter;
    bld_compiler_type* type;

    match = 0;
    iter = iter_set(handles);
    while (iter_next(&iter, (void**) &type)) {
        match = match || compiler_type_file_is_header(*type, name);
    }

    return match;
}

int compile_to_object(bld_compiler_type type, bld_string* compiler, bld_string* flags, bld_path* file_path, bld_path* object_path) {
    switch (type) {
        case (BLD_COMPILER_GCC):
            return compile_to_object_gcc(compiler, flags, file_path, object_path);
        case (BLD_COMPILER_CLANG):
            return compile_to_object_clang(compiler, flags, file_path, object_path);
        case (BLD_COMPILER_AMOUNT):
            break;
    }

    log_fatal("compile_to_object: unknown type %d", type);
    return 0;
}

int compiler_type_file_is_implementation(bld_compiler_type type, bld_string* name) {
    switch (type) {
        case (BLD_COMPILER_GCC):
            return compiler_file_is_implementation_gcc(name);
        case (BLD_COMPILER_CLANG):
            return compiler_file_is_implementation_clang(name);
        case (BLD_COMPILER_AMOUNT):
            break;
    }

    log_fatal("compiler_file_is_implementation: unknown type %d", type);
    return 0;
}

int compiler_type_file_is_header(bld_compiler_type type, bld_string* name) {
    switch (type) {
        case (BLD_COMPILER_GCC):
            return compiler_file_is_header_gcc(name);
        case (BLD_COMPILER_CLANG):
            return compiler_file_is_header_clang(name);
        case (BLD_COMPILER_AMOUNT):
            break;
    }

    log_fatal("compiler_file_is_header: unknown type %d", type);
    return 0;
}

bld_language_type compiler_file_language(bld_compiler_type type, bld_string* name) {
    switch (type) {
        case (BLD_LANGUAGE_C):
            return compiler_file_language_gcc(name);
        case (BLD_LANGUAGE_CPP):
            return compiler_file_language_clang(name);
        case (BLD_LANGUAGE_AMOUNT):
            log_fatal(LOG_FATAL_PREFIX "unknown language");
    }

    log_fatal(LOG_FATAL_PREFIX "not in range");
    return BLD_LANGUAGE_AMOUNT; /* unreachable */
}
