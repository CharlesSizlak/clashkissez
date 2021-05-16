#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>

#ifndef NDEBUG
#define DEBUG_PRINTF(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) (void)fmt
#endif

#ifndef NDEBUG
#define ERROR_PRINTF(fmt, ...) fprintf(stderr, "\x1B[31merror\x1B[0m:%s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define ERROR_PRINTF(fmt, ...) fprintf(stderr, "\x1B[31merror\x1B[0m:" fmt "\n", ##__VA_ARGS__)
#endif

#endif