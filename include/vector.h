#ifndef _VECTOR_H
#define _VECTOR_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct int_vector_t {
    size_t count;
    int *values;
    size_t capacity;
} int_vector_t;

/**
 * @brief Returns a resizeable vector of ints. Should be paired with a int_vector_free
 * @param vector_capacity A starting size for the int_vector_t
 */
// TODO Add on to any briefs for constructors that are paired with a destructor, that they are paired
int_vector_t *int_vector_new(size_t vector_capacity);

/**
 * @brief Appends a value to the vector
 */
void int_vector_append(int_vector_t *vector, int value);

/**
 * @brief Removes and places in the output the rightmost value in the vector
 * 
 * @param value This is an output parameter
 * @return true on success
 * @return false if the vector is empty
 */
bool int_vector_pop(int_vector_t *vector, int *value);

/**
 * @brief Removes the rightmost entry in the vector that matches the given int
 * 
 * @return true on success
 * @return false on failure to find a matching int
 */
bool int_vector_remove(int_vector_t *vector, int value);

/**
 * @brief Removes a value from the vector at the given index
 */
void int_vector_remove_index(int_vector_t *vector, size_t index);

/**
 * @brief Frees resources allocated for a vector
 */
void int_vector_free(int_vector_t *vector);

#endif
