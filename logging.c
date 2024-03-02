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

bld_log_level set_log_level(bld_log_level level) {
    bld_log_level old_level = log_level;
    log_level = level;
    return old_level;
}

void log_debug(const char* fmt, ...) {
    if (log_level > BLD_DEBUG) {return;}
    BLD_LOG_COL("[     DEBUG] ", BLD_BLUE);
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_info(const char* fmt, ...) {
    if (log_level > BLD_INFO) {return;}
    printf(     "[      INFO] ");
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_warn(const char* fmt, ...) {
    if (log_level > BLD_WARN) {return;}
    BLD_LOG_COL("[      WARN] ", BLD_ORANGE);
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_deprecated(const char* fmt, ...) {
    BLD_LOG_COL("[DEPRECATED] ", BLD_ORANGE);
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_error(const char* fmt, ...) {
    BLD_LOG_COL("[     ERROR] ", BLD_RED);
    BLD_LOG_STRING(fmt);
    printf("\n");
}

void log_fatal(const char* fmt, ...) {
    BLD_LOG_COL("[     FATAL] ", BLD_RED);
    BLD_LOG_STRING(fmt);
    printf("\n");
    exit(1);
}
