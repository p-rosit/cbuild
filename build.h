#ifndef BUILD_H
#define BUILD_H

#include <stdio.h>

#define log_fatal(...) do {printf("[FATAL] " __VA_ARGS__); printf("\n"); exit(1);} while (0)
#define log_warn(...)  do {printf("[ WARN] " __VA_ARGS__); printf("\n");} while (0) 
#define log_info(...)  do {printf("[ INFO] " __VA_ARGS__); printf("\n");} while (0) 

#endif
