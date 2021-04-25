#ifndef _LOOP_H
#define _LOOP_H

#include <hash.h>
#include <signal.h>
#include <time.h>

typedef struct loop_t {
    /*
    an amount of these, it will be static. 
    fd_callback_f;
    */
    int epoll_fd;
    kh_map_t *fd_map;
    kh_map_t *sig_map;
    bool running;
    sigset_t sigset;
} loop_t;

typedef enum event_e {
    READ_EVENT,
    WRITE_EVENT,
    READ_WRITE_EVENT,
} event_e;

typedef void (*fd_callback_f)(loop_t *, event_e, int);

typedef void (*signal_callback_f)(loop_t *, int);

typedef struct poll_item_t {
    fd_callback_f cb;
    int fd;
} poll_item_t;

void loop_init(loop_t *loop);

int loop_add_fd(
    loop_t *loop,
    int fd,
    event_e event,
    fd_callback_f cb
);

void loop_remove_fd(loop_t *loop, int fd);

int loop_add_signal(loop_t *loop, int signum, signal_callback_f cb);

void loop_remove_signal(loop_t *loop, int signum);

void loop_add_timer(loop_t *loop, struct timespec);

void loop_remove_timer(loop_t *loop);

int loop_run(loop_t *loop);

void loop_fini(loop_t *loop);

#endif