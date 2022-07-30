#ifndef _TIME_HEAP_H
#define _TIME_HEAP_H

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <khash.h>

KHASH_MAP_INIT_INT64(tmap, size_t)

typedef struct time_element_t {
    size_t id;
    struct timespec timer;
} time_element_t;

typedef struct time_heap_t {
    size_t total_elements;
    size_t capacity;
    time_element_t *elements;
    kh_tmap_t *hashtable;
    size_t next_id;
} time_heap_t;

// Note: All times are recorded in and checked against CLOCK_MONOTONIC

/**
 * @brief Allocates resources for a time_heap_t
 */
time_heap_t *heap_create(void);

/**
 * @brief Adds a timer that will expire at the time snapshot of the element given
 * 
 * @return A numerical ID for the timer within the time_heap
 */
size_t heap_add (time_heap_t *heap, struct timespec *element);

/**
 * @brief  Removes a timer by its ID
 */
void heap_remove(time_heap_t *heap, size_t id);

/**
 * @brief Checks whether any timers have expired
 */
bool heap_peek_head(time_heap_t *heap);

/**
 * @brief Checks whether a specific timer has expired or not
 */
bool heap_peek_by_id(time_heap_t *heap, size_t id);

/**
 * @brief Updates the identified timer to expire at the given struct timespec
 */
// TODO Add const to things where it'd help people understand how to use things
void heap_update(time_heap_t *heap, size_t id, const struct timespec *element);

/**
 * @brief Copies out a struct timespec from the heap
 * 
 * @param output A pointer passed to be the output for a snapshot in CLOCK_MONOTONIC time
 */
void heap_get(time_heap_t *heap, size_t id, struct timespec *output);

/**
 * @brief Frees the resources allocated for a given time_heap_t
 */
void heap_free(time_heap_t *heap);

/**
 * @brief Removes the timer with the smallest amount of time on CLOCK_MONOTONIC time
 */
void heap_extract(time_heap_t *heap);

#endif
