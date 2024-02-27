#ifndef BUILD_H
#define BUILD_H

#include <stdio.h>

#ifndef LOG_LEVEL
    #define LOG_LEVEL (1)
#endif

#define log_fatal(...) \
    do {\
        if (LOG_LEVEL <= 3) { \
            printf("\033[0;31m[FATAL]\033[0m " __VA_ARGS__);\
            printf("\n"); exit(1);\
        } \
    } while (0)

#define log_warn(...) \
    do { \
        if (LOG_LEVEL <= 2) { \
            printf("\033[0;33m[ WARN]\033[0m " __VA_ARGS__);\
            printf("\n");\
        } \
    } while (0) 

#define log_info(...) \
    do {\
        if (LOG_LEVEL <= 1) { \
            printf("[ INFO] " __VA_ARGS__); \
            printf("\n");\
        } \
    } while (0) 

#define log_debug(...) \
    do {\
        if (LOG_LEVEL <= 0) { \
            printf("\033[34m[DEBUG]\033[0m " __VA_ARGS__); \
            printf("\n");\
        } \
    } while (0) 

#endif
