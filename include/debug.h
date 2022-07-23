#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>

#ifndef NDEBUG
/**
 * @brief A special version of printf that will only be called in debug mode.
 * Adds a newline at the end.
 */
#define DEBUG_PRINTF(fmt, ...) fprintf(stderr, "%s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) (void)fmt
#endif

#ifndef NDEBUG
/**
 * @brief Prints the line number where an error code. 
 * In production it only states that an error has occured.
 */
#define ERROR_PRINTF(fmt, ...) fprintf(stderr, "\x1B[31merror\x1B[0m:%s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define ERROR_PRINTF(fmt, ...) fprintf(stderr, "\x1B[31merror\x1B[0m:" fmt "\n", ##__VA_ARGS__)
#endif

#endif
