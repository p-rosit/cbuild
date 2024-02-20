#ifndef BUILD_H
#define BUILD_H

#include <stdio.h>

#define log_fatal(...) do {printf("\033[0;31m[FATAL]\033[0m " __VA_ARGS__); printf("\n"); exit(1);} while (0)
#define log_warn(...)  do {printf("\033[0;33m[ WARN]\033[0m " __VA_ARGS__); printf("\n");} while (0) 
#define log_info(...)  do {printf("[ INFO] " __VA_ARGS__); printf("\n");} while (0) 

#endif
