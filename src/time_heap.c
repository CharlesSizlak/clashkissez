#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include "time_heap.h"

static bool timer_gt(time_heap_t *heap, size_t idx_a, size_t idx_b)
{
    struct timespec *a = &heap->elements[idx_a].timer;
    struct timespec *b = &heap->elements[idx_b].timer;
    return (a->tv_sec > b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec));
}

static void timer_swp(time_heap_t *heap, size_t idx_a, size_t idx_b)
{
    time_element_t *a = &heap->elements[idx_a];
    time_element_t *b = &heap->elements[idx_b];
    time_element_t temp;
    memcpy(&temp, a, sizeof(time_element_t));
    memcpy(a, b, sizeof(time_element_t));
    memcpy(b, &temp, sizeof(time_element_t));
}

static void thash_add(kh_tmap_t *hashtable, size_t key, size_t value)
{
    int ret;
    khint_t i = kh_put_tmap(hashtable, key, &ret);
    kh_val(hashtable, i) = value;
}

size_t thash_get(kh_tmap_t *hashtable, size_t key)
{
    khint_t i = kh_get_tmap(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return SIZE_MAX;
    }
    size_t value = kh_val(hashtable, i);
    return value;
}

time_heap_t *heap_create(void)
{
    //TODO check if malloc or calloc fail Kappa
    time_heap_t *heap = malloc(sizeof(time_heap_t));
    heap->total_elements = 0;
    heap->capacity = 32;
    heap->elements = calloc(heap->capacity, sizeof(time_element_t));
    heap->hashtable = kh_init_tmap();
    heap->next_id = 0;
    return heap;
}

size_t heap_add (time_heap_t *heap, struct timespec *element)
{
    if (heap->total_elements == heap->capacity) {
        heap->capacity *= 2;
        heap->elements = realloc(heap->elements, sizeof(time_element_t) * heap->capacity);
    }
    memcpy(&heap->elements[heap->total_elements].timer, element, sizeof(struct timespec));
    heap->elements[heap->total_elements].id = heap->next_id;
    size_t index = heap->total_elements;
    size_t id = heap->next_id;
    heap->next_id++;
    heap->total_elements++;
    while (true)
    {
        size_t parent_index = (index - 1) / 2;
        if (index == 0 || timer_gt(heap, index, parent_index)) 
        {
            thash_add(heap->hashtable, id, index);
            return id;
        }
        else
        {
            thash_add(heap->hashtable, heap->elements[parent_index].id, index);
            timer_swp(heap, index, parent_index);
            index = parent_index;
        }
    }
}

void heap_remove(time_heap_t *heap, size_t id)
{
    // swap index with last position in the heap
    size_t index = thash_get(heap->hashtable, id);
    if (index == SIZE_MAX)
    {
        // If we hit this, that means that this key didn't actually exist
        // Make DEBUG_PRINTF work
        DEBUG_PRINTF("Failed to heap_remove.");
        return;
    }

    // check the children of the inheritor of the index, down heapify until we're valid

    // :)
}

bool heap_peek(time_heap_t *heap)
{
    // Check to see if the timer on top of the heap has expired
    return;
}

void heap_update(time_heap_t *heap, size_t id, struct timespec *element)
{
}

void heap_free(time_heap_t *heap)
{
}