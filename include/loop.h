#ifndef _LOOP_H
#define _LOOP_H

#include <hash.h>
#include <signal.h>
#include <time.h>
#include "time_heap.h"

typedef struct loop_t {
    /*
    an amount of these, it will be static. 
    fd_callback_f;
    */
    int epoll_fd;
    kh_map_t *fd_map;
    kh_map_t *sig_map;
    kh_sz_map_t *timer_map;
    bool running;
    sigset_t sigset;
    time_heap_t *heap;
} loop_t;

typedef enum event_e {
    READ_EVENT,
    WRITE_EVENT,
    READ_WRITE_EVENT,
    ERROR_EVENT
} event_e;

typedef struct cb_data_t {
    void *cb;
    void *data;
} cb_data_t;

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

int loop_add_fd(
    loop_t *loop,
    int fd,
    event_e event,
    fd_callback_f cb,
    void *data
);

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

int loop_run(loop_t *loop);

void loop_fini(loop_t *loop);

#endif