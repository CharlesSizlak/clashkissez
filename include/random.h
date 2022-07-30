#ifndef _RANDOM_H
#define _RANDOM_H

#include <stdlib.h>

/**
 * @brief Used to make rand_get_bytes useable
 */
void rand_init(void);

/**
 * @brief Generates cryptographically secure random bytes
 */
void rand_get_bytes(void *buffer, size_t buffer_length);

/**
 * @brief Cleans up resources used in rand_init
 */
void rand_cleanup(void);

#endif
