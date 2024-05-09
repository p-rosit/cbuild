#ifndef LOGGING_H
#define LOGGING_H

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

void log_debug(const char*, ...);
void log_info(const char*, ...);
void log_warn(const char*, ...);
void log_deprecated(const char*, ...);
void log_error(const char*, ...);
void log_fatal(const char*, ...);

#endif
