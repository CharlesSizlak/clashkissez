#ifndef _TIME_HEAP_H
#define _TIME_HEAP_H

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <khash.h>

KHASH_MAP_INIT_INT64(tmap, size_t);

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

time_heap_t *heap_create(void);

size_t heap_add (time_heap_t *heap, struct timespec *element);

void heap_remove(time_heap_t *heap, size_t id);

bool heap_peek(time_heap_t *heap);

void heap_update(time_heap_t *heap, size_t id, struct timespec *element);

void heap_free(time_heap_t *heap);

#endif