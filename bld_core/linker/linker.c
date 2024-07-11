#include "../logging.h"
#include "linker.h"
#include "gcc.h"
#include "clang.h"
#include "zig.h"

bld_array linker_get_available(void) {
    bld_array linkers = array_new(sizeof(bld_string));

    array_push(&linkers, &bld_linker_string_gcc);
    array_push(&linkers, &bld_linker_string_clang);
    array_push(&linkers, &bld_linker_string_zig);

    return linkers;
}

bld_linker_type linker_get_mapping(bld_string* name) {
    size_t i;
    bld_string* linkers[] = {
        &bld_linker_string_gcc,
        &bld_linker_string_clang,
        &bld_linker_string_zig
    };
    bld_linker_type types[] = {
        BLD_LINKER_GCC,
        BLD_LINKER_CLANG,
        BLD_LINKER_ZIG
    };

    if (sizeof(linkers) / sizeof(*linkers) != sizeof(types) / sizeof(*types)) {
        log_fatal(LOG_FATAL_PREFIX "internal error");
    }

    for (i = 0; i < sizeof(types) / sizeof(*types); i++) {
        if (string_eq(name, linkers[i])) {
            return types[i];
        }
    }

    return BLD_LINKER_AMOUNT;
}

bld_string* linker_get_string(bld_linker_type type) {
    bld_string* linkers[] = {
        &bld_linker_string_gcc,
        &bld_linker_string_clang,
        &bld_linker_string_zig
    };

    if (sizeof(linkers) / sizeof(*linkers) != BLD_LINKER_AMOUNT) {
        log_fatal(LOG_FATAL_PREFIX "incorrect amount of strings");
    }

    if (type < 0 || type >= BLD_LINKER_AMOUNT) {
        log_fatal(LOG_FATAL_PREFIX "type %d cannot be converted to handle", type);
    }

    return linkers[type];
}

int linker_executable_make(bld_linker* linker, bld_path* root, bld_array* files, bld_array* flags, bld_path* path) {
    switch (linker->type) {
        case (BLD_LINKER_GCC):
            return linker_executable_make_gcc(linker, root, files, flags, path);
        case (BLD_LINKER_CLANG):
            return linker_executable_make_clang(linker, root, files, flags, path);
        case (BLD_LINKER_ZIG):
            return linker_executable_make_zig(linker, root, files, flags, path);
        case (BLD_LINKER_AMOUNT):
            log_fatal(LOG_FATAL_PREFIX "invalid type");
    }

    log_fatal(LOG_FATAL_PREFIX "outside range");
    return -1; /* unreachable */
}
