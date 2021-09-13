#ifndef _VECTOR_H
#define _VECTOR_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct int_vector_t {
    size_t count;
    int *values;
    size_t capacity;
} int_vector_t;

int_vector_t *int_vector_new(size_t vector_capacity);

void int_vector_append(int_vector_t *vector, int value);

bool int_vector_pop(int_vector_t *vector, int *value);

bool int_vector_remove(int_vector_t *vector, int value);

void int_vector_free(int_vector_t *vector);

#endif
