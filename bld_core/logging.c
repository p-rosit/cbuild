#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "logging.h"

#define BLD_RED         "\033[0;31m"
#define BLD_ORANGE      "\033[0;33m"
#define BLD_BLUE        "\033[34m"
#define BLD_RESET_COL   "\033[0m"

#define BLD_LOG_STRING(fmt) \
    do {                        \
        va_list valist;         \
        va_start(valist, fmt);  \
        vprintf(fmt, valist);   \
        va_end(valist);         \
    } while (0)

#define BLD_LOG_COL(str, col) \
    do {                        \
        printf(col);            \
        printf(str);            \
        printf(BLD_RESET_COL);  \
    } while (0)

int log_level = BLD_DEBUG;

bld_string bld_log_level_debug = STRING_COMPILE_TIME_PACK("debug");
bld_string bld_log_level_dinfo = STRING_COMPILE_TIME_PACK("dinfo");
bld_string bld_log_level_info = STRING_COMPILE_TIME_PACK("info");
bld_string bld_log_level_warn = STRING_COMPILE_TIME_PACK("warn");
bld_string bld_log_level_deprecated = STRING_COMPILE_TIME_PACK("deprecrated");
bld_string bld_log_level_error = STRING_COMPILE_TIME_PACK("error");
bld_string bld_log_level_fatal = STRING_COMPILE_TIME_PACK("fatal");

bld_log_level set_log_level(bld_log_level level) {
    bld_log_level old_level = log_level;
    log_level = level;
    return old_level;
}

bld_string* log_level_to_string(bld_log_level level) {
    switch (level) {
        case (BLD_DEBUG):
            return &bld_log_level_debug;
        case (BLD_DEBUG_INFO):
            return &bld_log_level_dinfo;
        case (BLD_INFO):
            return &bld_log_level_info;
        case (BLD_WARN):
            return &bld_log_level_warn;
        case (BLD_DEPRECATED):
            return &bld_log_level_deprecated;
        case (BLD_ERROR):
            return &bld_log_level_error;
        case (BLD_FATAL):
            return &bld_log_level_fatal;
    }

    log_fatal(LOG_FATAL_PREFIX "unreachable error");
    return NULL; /* unreachable */
}

bld_log_level log_level_from_string(bld_string* str) {
    bld_log_level i;
    bld_string* level[] = {
        &bld_log_level_debug,
        &bld_log_level_dinfo,
        &bld_log_level_info,
        &bld_log_level_warn,
        &bld_log_level_deprecated,
        &bld_log_level_error,
        &bld_log_level_fatal
    };

    for (i = BLD_DEBUG; i < sizeof(level) / sizeof(*level); i++) {
        if (string_eq(str, level[i])) {
            return i;
        }
    }

    return -1;
}

void log_debug(const char* fmt, ...) {
    if (log_level > BLD_DEBUG) {return;}
    BLD_LOG_COL("[DEBUG] ", BLD_BLUE);
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_dinfo(const char* fmt, ...) {
    if (log_level > BLD_DEBUG_INFO) {return;}
    printf(     "[DINFO] ");
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_info(const char* fmt, ...) {
    if (log_level > BLD_INFO) {return;}
    printf(     "[ INFO] ");
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_warn(const char* fmt, ...) {
    if (log_level > BLD_WARN) {return;}
    BLD_LOG_COL("[ WARN] ", BLD_ORANGE);
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_deprecated(const char* fmt, ...) {
    BLD_LOG_COL("[DEPRECATED] ", BLD_ORANGE);
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_error(const char* fmt, ...) {
    BLD_LOG_COL("[ERROR] ", BLD_RED);
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_fatal(const char* fmt, ...) {
    BLD_LOG_COL("[FATAL] ", BLD_RED);
    BLD_LOG_STRING(fmt);
    printf("\n");
    exit(1);
}
