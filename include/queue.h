#ifndef _QUEUE_H
#define _QUEUE_H

#include <pthread.h>
#include <semaphore.h>

typedef struct queue_node_t {
    void *data;
    struct queue_node_t *next;
} queue_node_t;

typedef struct queue_t {
    queue_node_t *head;
    queue_node_t *tail;
    pthread_mutex_t lock;
    sem_t semaphore;
} queue_t;

/**
 * @brief Allocates resources for a threadsafe queue
 * 
 * @return A pointer to a new queue
 */
queue_t *queue_new(void);

/**
 * @brief Adds something to the queue and wakes up any threads waiting to dequeue
 */
void queue_enqueue(queue_t *queue, void *ptr);

/**
 * @brief Sleeps until something is ready to be dequeued, in which case it 
 * returns what was dequeued.
 */
void *queue_dequeue(queue_t *queue);

/**
 * @brief If there is something in the queue dequeues and returns it, otherwise
 * returns null.
 */
void *queue_try_dequeue(queue_t *queue);

/**
 * @brief Frees resources allocated for the queue
 */
void queue_free(queue_t *queue);

#endif
