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

queue_t *queue_new(void);

void queue_enqueue(queue_t *queue, void *ptr);

void *queue_dequeue(queue_t *queue);

void *queue_try_dequeue(queue_t *queue);

void queue_free(queue_t *queue);

#endif