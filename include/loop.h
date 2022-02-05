#ifndef _LOOP_H
#define _LOOP_H

#include <hash.h>
#include <signal.h>
#include <time.h>
#include "queue.h"
#include "time_heap.h"

typedef void *(*thread_f)(void *);

typedef struct loop_t {
    int epoll_fd;
    kh_map_t *fd_map;
    kh_map_t *sig_map;
    kh_sz_map_t *timer_map;
    kh_sz_map_t *paused_timer_map;
    bool running;
    sigset_t sigset;
    time_heap_t *heap;
    queue_t *pending_fd_callbacks;
    pthread_t thread_id;
} loop_t;

typedef enum event_e {
    READ_EVENT,
    WRITE_EVENT,
    READ_WRITE_EVENT,
    ERROR_EVENT,
    TRIGGER_EVENT
} event_e;

typedef struct cb_data_t {
    void *cb;
    void *data;
} cb_data_t;

typedef struct paused_timer_t {
    struct timespec pause_time;
    struct timespec expiration_time;
} paused_timer_t;

// Takes the event loop, an event_e which contains
// the type of event that occured along the fd, the fd, and data
typedef void (*fd_callback_f)(loop_t *, event_e, int, void *);

// Takes our event loop, the signal number, and data
typedef void (*signal_callback_f)(loop_t *, int, void *);

// Takes the event loop and data
typedef void (*timer_callback_f)(loop_t *, size_t, void *);

typedef struct poll_item_t {
    fd_callback_f cb;
    int fd;
    void *data;
} poll_item_t;

void loop_init(loop_t *loop);

void loop_add_fd(
    loop_t *loop,
    int fd,
    event_e event,
    fd_callback_f cb,
    void *data
);

void loop_trigger_fd(loop_t *loop, int fd, fd_callback_f cb, void *data);

void loop_remove_fd(loop_t *loop, int fd);

void loop_add_signal(loop_t *loop, 
    int signum, 
    signal_callback_f cb, 
    void *data
);

void loop_remove_signal(loop_t *loop, int signum);


size_t loop_add_timer(
    loop_t *loop, 
    struct timespec *timer, 
    timer_callback_f cb, 
    void *data
);

void loop_remove_timer(loop_t *loop, size_t id);

void loop_update_timer(loop_t *loop, size_t id, struct timespec *timer);

void loop_pause_timer(loop_t *loop, size_t id);

void loop_unpause_timer(loop_t *loop, size_t id);

int loop_get_time_remaining(loop_t *loop, size_t id);

int timer_subtract(struct timespec *a, struct timespec *b);

void timer_add(struct timespec *a, int milliseconds);

int loop_run(loop_t *loop);

void loop_fini(loop_t *loop);

#endif
