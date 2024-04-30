#ifndef LOGGING_H
#define LOGGING_H

typedef enum bit_log_level {
    BIT_DEBUG,
    BIT_INFO,
    BIT_WARN,
    BIT_DEPRECATED,
    BIT_ERROR,
    BIT_FATAL
} bit_log_level;

extern int log_level;

bit_log_level set_log_level(bit_log_level);

void log_debug(const char*, ...);
void log_info(const char*, ...);
void log_warn(const char*, ...);
void log_deprecated(const char*, ...);
void log_error(const char*, ...);
void log_fatal(const char*, ...);

#endif
