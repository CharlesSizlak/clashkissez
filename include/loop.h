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

typedef enum time_unit_e {
    TU_MILLISECONDS,
    TU_SECONDS,
    TU_MINUTES,
    TU_HOURS,
    TU_DAYS
} time_unit_e;

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

/**
 * @brief Initializes a given loop
 */
void loop_init(loop_t *loop);

/**
 * @brief Registers a file descriptor to be used as part of the event loop
 */
void loop_add_fd(
    loop_t *loop,
    int fd,
    event_e event,
    fd_callback_f cb,
    void *data
);

/**
 * @brief Called from a thread other than the loop's thread. Interrupts the
 * call to poll and queues an fd callback.
 */
void loop_trigger_fd(loop_t *loop, int fd, fd_callback_f cb, void *data);

/**
 * @brief Removes a fd from the event loop
 */
void loop_remove_fd(loop_t *loop, int fd);

/**
 * @brief Adds a callback to be triggered when the given signal comes in to the loop
 */
void loop_add_signal(loop_t *loop, 
    int signum, 
    signal_callback_f cb, 
    void *data
);

/**
 * @brief Removes a signal watch from the event loop.
 */
void loop_remove_signal(loop_t *loop, int signum);

/**
 * @brief Adds a timer to the loop that triggers the callback when the time is up
 * @param timer Uses CLOCK_MONOTONIC timestamps
 * @return An id that can be used to later reference this timer
 */
size_t loop_add_timer(
    loop_t *loop, 
    struct timespec *timer, 
    timer_callback_f cb, 
    void *data
);

/**
 * @brief Creates a timer that expires when you multiply time and unit together and add it to the current time.
 * @param time The number of time units to add to the current time for the timer
 * @param unit Defines what unit of time to multiply the parameter time by
 * @return An id that can be used to later reference this timer
 */
size_t loop_add_timer_relative(
    loop_t *loop, 
    int time, 
    time_unit_e unit,
    timer_callback_f cb, 
    void *data
);

// All these are pretty self explanatory
void loop_remove_timer(loop_t *loop, size_t id);
void loop_update_timer(loop_t *loop, size_t id, struct timespec *timer);
void loop_pause_timer(loop_t *loop, size_t id);
void loop_unpause_timer(loop_t *loop, size_t id);
int loop_get_time_remaining(loop_t *loop, size_t id);

/**
 * @brief Updates a timer relative to the absolute time of that timers expiration.
 * @param time The number of units to add or subtract to the existing timer
 * @param unit Defines what unit of time to multiply the parameter time by
 */
void loop_update_timer_relative(loop_t *loop, size_t id, int time, time_unit_e unit);

/**
 * @brief Returns the difference in milliseconds of a - b
 */
int timer_subtract(struct timespec *a, struct timespec *b);

/**
 * @brief Adds milliseconds to a given struct timespec
 */
void timer_add(struct timespec *a, int milliseconds);

/**
 * @brief Begins the event loop which will run until an event in the loop stops it
 */
int loop_run(loop_t *loop);

/**
 * @brief Cleans up resources allocated for the event loop
 */
void loop_fini(loop_t *loop);

#endif
