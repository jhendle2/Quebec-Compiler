#ifndef QUEBEC_COMMON_H
#define QUEBEC_COMMON_H

#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint;

#define ERRO(EXIT_CODE, ...) { printf("[ERRO] "); printf(__VA_ARGS__); printf("\n"); exit(EXIT_CODE); }
#define WARN(...)            { printf("[WARN] "); printf(__VA_ARGS__); printf("\n"); }
#define INFO(...)            { printf("[INFO] "); printf(__VA_ARGS__); printf("\n"); }

#endif /* QUEBEC_COMMON_H */