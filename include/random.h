#ifndef _RANDOM_H
#define _RANDOM_H

#include <stdlib.h>

void rand_init(void);

void rand_get_bytes(void *buffer, size_t buffer_length);

void rand_cleanup(void);

#endif
