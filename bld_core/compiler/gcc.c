#include "../logging.h"
#include "gcc.h"

bld_string bld_compiler_string_gcc = STRING_COMPILE_TIME_PACK("gcc");
bld_string bld_compiler_string_gpp = STRING_COMPILE_TIME_PACK("g++");

int compile_to_object_gcc(bld_string* compiler, bld_string* flags, bld_path* file_path, bld_path* object_path) {
    int code;
    bld_string cmd;

    cmd = string_copy(compiler);
    string_append_space(&cmd);

    string_append_string(&cmd, string_unpack(flags));
    string_append_space(&cmd);

    string_append_string(&cmd, path_to_string(file_path));
    string_append_string(&cmd, " -c -o ");
    string_append_string(&cmd, path_to_string(object_path));

    code = system(string_unpack(&cmd));

    string_free(&cmd);
    return code;
}

/* TODO: verify list of file extensions gcc can take */
int compiler_file_is_implementation_gcc(bld_string* name) {
    int match;
    size_t i;
    bld_string extension;
    char* types[] = {
        "c", "i", /* C */
        "ii", "cc", "cp", "cxx", "cpp", "CPP", "c++", "C", /* C++ */
        "m", "mi", /* Objective-C */
        "mm", "M", "mii", /* Objecive-C++ */
        "f", "fi", "for", "fpp", "ftn", "f90", "f95", "f03", "f08", "fii", "F", "FOR", "FPP", "FTN", "F90", "F95", "F03", "F08", /* Fortran */
        "go", /* go */
        "d", /* D */
        "adb", /* Ada */
        "s", "sx", "S" /* Assembly */
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

int compiler_file_is_header_gcc(bld_string* name) {
    int match;
    size_t i;
    bld_string extension;
    char* types[] = {
        "h", /* C */
        "hh", "H", "hp", "hxx", "hpp", "HPP", "h++", "tcc", /* C++ */
        "di", /* D */
        "ads", /* Ada */
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

bld_language_type compiler_file_language_gcc(bld_string* name) {
    bld_language_type type;
    bld_string file_extension;
    size_t amount[9] = {3, 16, 2, 3, 18, 1, 2, 2, 3};
    char* types[9][18] = {
        {"c", "i", "h"}, /* C */
        {"ii", "cc", "cp", "cxx", "cpp", "CPP", "c++", "C", "hh", "H", "hp", "hxx", "hpp", "HPP", "h++", "tcc"}, /* C++ */
        {"m", "mi"}, /* Objective-C */
        {"mm", "M", "mii"}, /* Objecive-C++ */
        {"f", "fi", "for", "fpp", "ftn", "f90", "f95", "f03", "f08", "fii", "F", "FOR", "FPP", "FTN", "F90", "F95", "F03", "F08"}, /* Fortran */
        {"go"}, /* go */
        {"d", "di"}, /* D */
        {"adb", "ads"}, /* Ada */
        {"s", "sx", "S"} /* Assembly */
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

    log_fatal(LOG_FATAL_PREFIX "gcc could not determine language of \"%s\"", string_unpack(name));
    return type; /* unreachable */
}
