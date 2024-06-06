#include <string.h>
#include "compiler.h"

bld_string compiler_get_file_extension(bld_string* name) {
    char* temp;

    temp = strrchr(string_unpack(name), '.');
    if (temp == NULL) {
        temp = name->chars + name->size - 1;
    }

    return string_pack(temp + 1);
}
