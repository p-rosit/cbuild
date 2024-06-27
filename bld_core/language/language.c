#include "../logging.h"
#include "language.h"
#include "c.h"
#include "cpp.h"

bld_array language_get_available(void) {
    bld_array languages = array_new(sizeof(bld_string));

    array_push(&languages, &bld_language_string_c);
    array_push(&languages, &bld_language_string_cpp);

    return languages;
}

bld_language_type language_get_mapping(bld_string* language) {
    size_t i;
    bld_string* languages[] = {
        &bld_language_string_c,
        &bld_language_string_cpp
    };
    bld_language_type types[] = {
        BLD_LANGUAGE_C,
        BLD_LANGUAGE_CPP
    };

    if (sizeof(languages) / sizeof(*languages) != sizeof(types) / sizeof(*types)) {
        log_fatal(LOG_FATAL_PREFIX "internal error");
    }

    for (i = 0; i < sizeof(types) / sizeof(*types); i++) {
        if (string_eq(language, languages[i])) {
            return types[i];
        }
    }

    return BLD_LANGUAGE_AMOUNT;
}

bld_string* language_get_string(bld_language_type type) {
    bld_string* languages[] = {
        &bld_language_string_c,
        &bld_language_string_cpp
    };

    if (sizeof(languages) / sizeof(*languages) != BLD_COMPILER_AMOUNT) {
        log_fatal(LOG_FATAL_PREFIX "internal error");
    }

    if (type < 0 || type >= BLD_LANGUAGE_AMOUNT) {
        log_fatal(LOG_FATAL_PREFIX "type %d cannot be converted to language", type);
    }

    return languages[type];
}

int language_get_includes(bld_language_type type, bld_path* path, bld_file* file, bld_set* files) {
    switch (type) {
        case (BLD_LANGUAGE_C):
            return language_get_includes_c(path, file, files);
        case (BLD_LANGUAGE_CPP):
            return language_get_includes_cpp(path, file, files);
        case (BLD_LANGUAGE_AMOUNT):
            log_fatal(LOG_FATAL_PREFIX "internal error");
    }

    log_fatal(LOG_FATAL_PREFIX "unreachable error");
    return -1; /* unreachable */
}
