#include "gcc.h"

bld_string bld_compiler_string_gcc = STRING_COMPILE_TIME_PACK("gcc");
bld_string bld_compiler_string_gpp = STRING_COMPILE_TIME_PACK("g++");

int compile_to_object_gcc(bld_string* cmd, bld_path* location, bld_string* name) {
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
