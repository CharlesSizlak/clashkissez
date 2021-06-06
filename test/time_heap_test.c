#include "time_heap.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char *timespec_to_str(struct timespec *ts)
{
    static char buffer[4096];
    struct tm tm_s;
    gmtime_r(&ts->tv_sec, &tm_s);
    strftime(buffer, 4096, "%F %T", &tm_s);
    sprintf(buffer + strlen(buffer), ".%09ld", ts->tv_nsec);
    return buffer;
}


static void heap_print(time_heap_t *heap)
{
    for(size_t i = 0; i < heap->total_elements; i++)
    {
        time_element_t *element = &heap->elements[i];
        printf("The id of position %zu is %zu\n", i, element->id);
        printf("Val: %s\n", timespec_to_str(&element->timer));
    }
}

int main()
{
    time_heap_t *heap = heap_create();
    //printf("Testing What Happens When Empty: %i\n", heap_peek(heap));
    struct timespec current_time;
    for (size_t i = 0; i < 4; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        current_time.tv_sec += rand() % 1000;
        current_time.tv_sec -= rand() % 1000;
        heap_add(heap, &current_time);
    }
    heap_print(heap);
    heap_extract(heap);
    heap_print(heap);
    printf("Heap Peek Result: %i\n", heap_peek(heap));
    heap_free(heap);
}
