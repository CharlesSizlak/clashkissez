#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "debug.h"
#include "queue.h"

queue_t *queue_new(void) {
    queue_t *queue = calloc(1, sizeof(queue_t));
    sem_init(&queue->semaphore, false, 0);
    if (pthread_mutex_init(&queue->lock, NULL) != 0)
    {
        return NULL;
    }
    return queue;
}

void queue_enqueue(queue_t *queue, void *ptr) {
    queue_node_t *new_node = calloc(1, sizeof(queue_node_t));
    new_node->data = ptr;
    pthread_mutex_lock(&queue->lock);
    if (queue->head == NULL) {
        queue->head = new_node;
        queue->tail = new_node;
    }
    else {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    pthread_mutex_unlock(&queue->lock);
    sem_post(&queue->semaphore);
}

void *queue_dequeue(queue_t *queue) {
    while (true) {
        sem_wait(&queue->semaphore);
        pthread_mutex_lock(&queue->lock);
        if (queue->head == NULL) {
            pthread_mutex_unlock(&queue->lock);
            continue;
        }
        void *data = queue->head->data;
        queue_node_t *next = queue->head->next;
        queue_node_t *old_head = queue->head;
        queue->head = next;
        pthread_mutex_unlock(&queue->lock);
        free(old_head);
        return data;
    }
}

void *queue_try_dequeue(queue_t *queue) {
    sem_trywait(&queue->semaphore);
    pthread_mutex_lock(&queue->lock);
    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }
    void *data = queue->head->data;
    queue_node_t *next = queue->head->next;
    queue_node_t *old_head = queue->head;
    queue->head = next;
    pthread_mutex_unlock(&queue->lock);
    free(old_head);
    return data;
}

void queue_free(queue_t *queue) {
    assert(queue->head == NULL);
    pthread_mutex_destroy(&queue->lock);
    sem_destroy(&queue->semaphore);
    free(queue);
}
