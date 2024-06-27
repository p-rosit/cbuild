#include "../logging.h"
#include "clang.h"

bld_string bld_compiler_string_clang = STRING_COMPILE_TIME_PACK("clang");

int compile_to_object_clang(bld_string* cmd, bld_path* location, bld_string* name) {
    int code;
    bld_path file;

    file = path_copy(location);
    path_append_string(&file, string_unpack(name));

    string_append_string(cmd, " -c -o ");
    string_append_string(cmd, path_to_string(&file));

    code = system(string_unpack(cmd));

    path_free(&file);
    return code;
}

/* TODO: figure out which file extensions clang can take */
int compiler_file_is_implementation_clang(bld_string* name) {
    int match;
    size_t i;
    bld_string extension;
    char* types[] = {
        "c", "i", /* C */
        "ii", "cc", "cp", "cxx", "cpp", "CPP", "c++", "C", /* C++ */
        "m", "mi", /* Objective-C */
        "mm", "mii", /* Objective-C++ */
        "s" /* Assembly */
    };

    extension = compiler_get_file_extension(name);

    match = 0;
    for (i = 0; i < sizeof(types) / sizeof(*types); i++) {
        bld_string temp;

        temp = string_pack(types[i]);
        match = match || string_eq(&extension, &temp);
    }

    return match;
}

int compiler_file_is_header_clang(bld_string* name) {
    int match;
    size_t i;
    bld_string extension;
    char* types[] = {
        "h",
        "hh", "H", "hp", "hxx", "hpp", "HPP", "h++", "tcc",
    };

    extension = compiler_get_file_extension(name);

    match = 0;
    for (i = 0; i < sizeof(types) / sizeof(*types); i++) {
        bld_string temp;

        temp = string_pack(types[i]);
        match = match || string_eq(&extension, &temp);
    }

    return match;
}

bld_language_type compiler_file_language_clang(bld_string* name) {
    bld_language_type type;
    bld_string file_extension;
    size_t amount[2] = {3, 16};
    char* types[2][16] = {
        {"c", "i", "h"}, /* C */
        {"ii", "cc", "cp", "cxx", "cpp", "CPP", "c++", "C", "hh", "H", "hp", "hxx", "hpp", "HPP", "h++", "tcc"}, /* C++ */
    };

    file_extension = compiler_get_file_extension(name);

    for (type = BLD_LANGUAGE_C; type < BLD_LANGUAGE_AMOUNT; type++) {
        size_t i;
        bld_string temp;

        for (i = 0; i < amount[type]; i++) {
            temp = string_pack(types[type][i]);
            if (string_eq(&file_extension, &temp)) {
                return type;
            }
        }
    }

    log_fatal(LOG_FATAL_PREFIX "clang could not determine language of \"%s\"", string_unpack(name));
    return type;
}
