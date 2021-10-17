#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"

int_vector_t *int_vector_new(size_t vector_capacity) {
    if (vector_capacity == 0) {
        return NULL;
    }
    int_vector_t *vector = malloc(sizeof(int_vector_t));
    vector->capacity = vector_capacity;
    vector->count = 0;
    vector->values = malloc(sizeof(int) * vector_capacity);
    return vector;
}

void int_vector_append(int_vector_t *vector, int value) {
    if (vector->count + 1 > vector->capacity) {
        vector->capacity *= 2;
        vector->values = realloc(vector->values, sizeof(int) * vector->capacity);
    }
    vector->values[vector->count++] = value;
}

bool int_vector_pop(int_vector_t *vector, int *value) {
    if (vector->count == 0) {
        return false;
    }
    *value = vector->values[--vector->count];
    if (vector->count < vector->capacity / 3) {
        vector->capacity /= 2;
        vector->values = realloc(vector->values, sizeof(int) * vector->capacity);
    }
    return true;
}

bool int_vector_remove(int_vector_t *vector, int value) {
    for (size_t i = vector->count; i > 0; i--) {
        if (vector->values[i - 1] == value) {
            memmove(vector->values + i - 1, vector->values + i, vector->count - (i - 1));
            vector->count -= 1;
            return true;
        }
    }
    return false;
}

void int_vector_remove_index(int_vector_t *vector, size_t index) {
    if (index >= vector->count) {
        return;
    }
    vector->count -= 1;
    memmove(vector->values + index, vector->values + index + 1, vector->count - index);
}

void int_vector_free(int_vector_t *vector) {
    free(vector->values);
    free(vector);
}
