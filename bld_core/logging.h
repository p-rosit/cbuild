#ifndef LOGGING_H
#define LOGGING_H
#include "dstr.h"

#define LOG_PANIC_EVAL(x) LOG_PANIC_EVAL_(x)
#define LOG_PANIC_EVAL_(x) #x
#define LOG_FATAL_PREFIX __FILE__ ":" LOG_PANIC_EVAL(__LINE__) ", "

typedef enum bld_log_level {
    BLD_DEBUG,
    BLD_INFO,
    BLD_WARN,
    BLD_DEPRECATED,
    BLD_ERROR,
    BLD_FATAL
} bld_log_level;

extern int log_level;

bld_log_level set_log_level(bld_log_level);
bld_string* log_level_to_string(bld_log_level);
bld_log_level log_level_from_string(bld_string*);

void log_debug(const char*, ...);
void log_info(const char*, ...);
void log_warn(const char*, ...);
void log_deprecated(const char*, ...);
void log_error(const char*, ...);
void log_fatal(const char*, ...);

#endif
