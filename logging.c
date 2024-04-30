#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "logging.h"

#define BIT_RED         "\033[0;31m"
#define BIT_ORANGE      "\033[0;33m"
#define BIT_BLUE        "\033[34m"
#define BIT_RESET_COL   "\033[0m"

#define BIT_LOG_STRING(fmt) \
    do {                        \
        va_list valist;         \
        va_start(valist, fmt);  \
        vprintf(fmt, valist);   \
        va_end(valist);         \
    } while (0)

#define BIT_LOG_COL(str, col) \
    do {                        \
        printf(col);            \
        printf(str);            \
        printf(BIT_RESET_COL);  \
    } while (0)

int log_level = BIT_DEBUG;

bit_log_level set_log_level(bit_log_level level) {
    bit_log_level old_level = log_level;
    log_level = level;
    return old_level;
}

void log_debug(const char* fmt, ...) {
    if (log_level > BIT_DEBUG) {return;}
    BIT_LOG_COL("[DEBUG] ", BIT_BLUE);
    BIT_LOG_STRING(fmt);
    printf("\n");
}

void log_info(const char* fmt, ...) {
    if (log_level > BIT_INFO) {return;}
    printf(     "[ INFO] ");
    BIT_LOG_STRING(fmt);
    printf("\n");
}

void log_warn(const char* fmt, ...) {
    if (log_level > BIT_WARN) {return;}
    BIT_LOG_COL("[ WARN] ", BIT_ORANGE);
    BIT_LOG_STRING(fmt);
    printf("\n");
}

void log_deprecated(const char* fmt, ...) {
    BIT_LOG_COL("[DEPRECATED] ", BIT_ORANGE);
    BIT_LOG_STRING(fmt);
    printf("\n");
}

void log_error(const char* fmt, ...) {
    BIT_LOG_COL("[ERROR] ", BIT_RED);
    BIT_LOG_STRING(fmt);
    printf("\n");
}

void log_fatal(const char* fmt, ...) {
    BIT_LOG_COL("[FATAL] ", BIT_RED);
    BIT_LOG_STRING(fmt);
    printf("\n");
    exit(1);
}
