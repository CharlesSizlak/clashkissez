#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "loop.h"
#include "hash.h"
#include "time_heap.h"
#include "queue.h"
#include "debug.h"

#define SECONDS_PER_HOUR (60 * 60)
#define SECONDS_PER_DAY (24 * 60 * 60)

static bool signals_received[_NSIG];

/*
Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.

int
timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
  Perform the carry for the later subtraction by updating y.
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

        Compute the time remaining to wait.
        tv_usec is certainly positive.
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  // Return 1 if result is negative.
  return x->tv_sec < y->tv_sec;
}
*/

static void tu_modify_timer(struct timespec *timer, int time, time_unit_e unit) {
    switch (unit)
    {
    case TU_MILLISECONDS:
        timer->tv_sec += time / 1000;
        timer->tv_nsec += (time % 1000) * 1000000;
        if (timer->tv_nsec < 0) {
            timer->tv_sec -= 1;
            timer->tv_nsec += 1000000000;
        }
        if (timer->tv_nsec >= 1000000000) {
            timer->tv_sec += 1;
            timer->tv_nsec -= 1000000000;
        }
        break;
    
    case TU_SECONDS:
        timer->tv_sec += time;
        break;
    
    case TU_MINUTES:
        timer->tv_sec += time * 60;
        break;
    
    case TU_HOURS:
        timer->tv_sec += time * SECONDS_PER_HOUR;
        break;
    
    case TU_DAYS:
        timer->tv_sec += time * SECONDS_PER_DAY;
        break;
    }
}

void signal_caught(int signum) 
{
    signals_received[signum] = true;
}

void loop_init(loop_t *loop)
{
    loop->epoll_fd = epoll_create1(0);
    loop->fd_map = kh_init_map();
    loop->sig_map = kh_init_map();
    loop->timer_map = kh_init_sz_map();
    loop->paused_timer_map = kh_init_sz_map();
    sigemptyset(&loop->sigset);
    loop->heap = heap_create();
    loop->pending_fd_callbacks = queue_new();
    loop->running = true;
    loop->thread_id = pthread_self();
}

void loop_add_fd(loop_t *loop, int fd, event_e event,
        fd_callback_f cb, void *data)
{
    struct epoll_event ev;
    switch (event)
    {
    case READ_EVENT:
        ev.events = EPOLLIN;
        break;
    
    case WRITE_EVENT:
        ev.events = EPOLLOUT;
        break;

    case READ_WRITE_EVENT:
        ev.events = EPOLLIN | EPOLLOUT;
        break;
    default:
        DEBUG_PRINTF("Invalid event passed");
        ev.events = 0;
        break;
    }
    
    if (hash_contains(loop->fd_map, fd)) {
        poll_item_t *poll_item = hash_get(loop->fd_map, fd);
        poll_item->cb = cb;
        poll_item->data = data;
        ev.data.ptr = poll_item;
        epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    }
    else {
        poll_item_t *poll_item = malloc(sizeof(poll_item_t));
        poll_item->cb = cb;
        poll_item->fd = fd;
        poll_item->data = data;
        ev.data.ptr = poll_item;
        epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
        hash_add(loop->fd_map, fd, poll_item);
    }
}

void loop_remove_fd(loop_t *loop, int fd)
{
    poll_item_t *poll_item = hash_remove(loop->fd_map, fd);
    free(poll_item);
    epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void loop_trigger_fd(loop_t *loop, int fd, fd_callback_f cb, void *data)
{
    poll_item_t *poll_item = malloc(sizeof(poll_item_t));
    poll_item->cb = cb;
    poll_item->fd = fd;
    poll_item->data = data;
    queue_enqueue(loop->pending_fd_callbacks, poll_item);
    pthread_kill(loop->thread_id, SIGUSR1);
}

void loop_add_signal(loop_t *loop, int signum, 
    signal_callback_f cb, void *data)
{
    sigaddset(&loop->sigset, signum);
    cb_data_t *cb_data = malloc(sizeof(cb_data_t));
    cb_data->data = data;
    cb_data->cb = (void*)cb;
    hash_add(loop->sig_map, signum, cb_data);
}

void loop_remove_signal(loop_t *loop, int signum)
{
    sigdelset(&loop->sigset, signum);
    free(hash_remove(loop->sig_map, signum));
}

 size_t loop_add_timer(loop_t *loop, struct timespec *timer, 
    timer_callback_f cb, void *data)
{
    size_t id = heap_add(loop->heap, timer);
    cb_data_t *cb_data = malloc(sizeof(cb_data_t));
    cb_data->data = data;
    cb_data->cb = (void*)cb;
    sz_hash_add(loop->timer_map, id, cb_data);
    return id;
}

size_t loop_add_timer_relative(loop_t *loop, int time, time_unit_e unit,
    timer_callback_f cb, void *data) 
{
    struct timespec timer;
    clock_gettime(CLOCK_MONOTONIC, &timer);
    tu_modify_timer(&timer, time, unit);
    return loop_add_timer(loop, &timer, cb, data);
}

void loop_remove_timer(loop_t *loop, size_t id)
{
    heap_remove(loop->heap, id);
    free(sz_hash_remove(loop->timer_map, id));
}

void loop_update_timer(loop_t *loop, size_t id, struct timespec *timer) 
{
    heap_update(loop->heap, id, timer);
}

void loop_pause_timer(loop_t *loop, size_t id) 
{
    static struct timespec infinite_timer = {
        .tv_nsec = 0xFFFFFFFF,
        .tv_sec = 0xFFFFFFFF
    };
    paused_timer_t *timer = malloc(sizeof(paused_timer_t));
    heap_get(loop->heap, id, &timer->expiration_time);
    clock_gettime(CLOCK_MONOTONIC, &timer->pause_time);
    sz_hash_add(loop->paused_timer_map, id, timer);
    heap_update(loop->heap, id, &infinite_timer);
}

void loop_unpause_timer(loop_t *loop, size_t id) 
{
    paused_timer_t *timer = sz_hash_remove(loop->paused_timer_map, id);
    // Figure out the difference between pause_time and current time
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    int milliseconds_diff = timer_subtract(&current_time, &timer->pause_time);
    // Add that difference to expiration time
    timer_add(&timer->expiration_time, milliseconds_diff);
    // Update the heap with the new expiration time
    heap_update(loop->heap, id, &timer->expiration_time);
    free(timer);
}

int loop_get_time_remaining(loop_t *loop, size_t id) 
{
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    struct timespec timer;
    heap_get(loop->heap, id, &timer);
    return timer_subtract(&timer, &current_time);
}

void loop_update_timer_relative(loop_t *loop, size_t id, int time, time_unit_e unit)
{
    struct timespec timer;
    heap_get(loop->heap, id, &timer);
    tu_modify_timer(&timer, time, unit);
    loop_update_timer(loop, id, &timer);
}

/**
 * Returns the difference in milliseconds of a - b
 */
int timer_subtract(struct timespec *a, struct timespec *b)
{
    // If there aren't enough nanoseconds, borrow from the
    // seconds field.
    if (a->tv_nsec < b->tv_nsec)
    {
        a->tv_sec--;
        a->tv_nsec += 1000000000;
    }

    int seconds = a->tv_sec - b->tv_sec;
    int nanoseconds = a->tv_nsec - b->tv_nsec;
    int milliseconds = seconds * 1000 + nanoseconds / 1000000;
    return milliseconds;
}

/**
 * Adds milliseconds into the given timespec
 */
void timer_add(struct timespec *a, int milliseconds)
{
    // Convert the milliseconds instead seconds and nanoseconds, add them to the respective tv_
    time_t seconds = milliseconds / 1000;
    milliseconds -= seconds * 1000;
    time_t nanoseconds = milliseconds * 1000000;
    a->tv_sec += seconds;
    a->tv_nsec += nanoseconds;
    if (a->tv_nsec >= 1000000000) {
        a->tv_nsec -= 1000000000;
        a->tv_sec += 1;
    }
}

int loop_run(loop_t *loop) 
{
    DEBUG_PRINTF("Entering loop!");
    struct epoll_event event;
    int epoll_return;
    size_t id;
    struct sigaction oldacts[_NSIG];
    struct sigaction act = {
        .sa_handler = signal_caught
    };
    sigemptyset(&act.sa_mask);
    int sleep = -1;
    bzero(oldacts, sizeof(struct sigaction) * _NSIG);

    // Setup signal handlers for every added signal
    for (khint_t i = 0; i != kh_end(loop->sig_map); i++)
    {
		if (!kh_exist(loop->sig_map, i)) {
            continue;
        }
        int signum = kh_key(loop->sig_map, i);
        if (sigaction(signum, &act, &oldacts[signum]) == -1)
        {
            DEBUG_PRINTF("sigaction failed: %s", strerror(errno));
            return -1;
        }
    }

    // Call epoll_wait until loop->running becomes false
    while (loop->running)
    {

        // Check for pending callbacks
        while (true) {
            poll_item_t *poll_item = queue_try_dequeue(loop->pending_fd_callbacks);
            if (poll_item == NULL) {
                break;
            }
            poll_item->cb(loop, TRIGGER_EVENT, poll_item->fd, poll_item->data);
            free(poll_item);
        }

        // Note for self, argument #4 is a int timer that blocks for
        // that many milliseconds, measured against CLOCK_MONOTONIC
        TIMER_CHECK:
        if (loop->heap->total_elements)
        {
            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            if (heap_peek_head(loop->heap))
            {
                // Pop off the top timer, call its cb
                // Then check the next timer and go until we're able to sleep
                id = loop->heap->elements[0].id;
                cb_data_t *timer_cb_data = sz_hash_get(loop->timer_map, id);
                timer_callback_f timer_cb = (timer_callback_f)timer_cb_data->cb;
                timer_cb(loop, id, timer_cb_data->data);
                if (heap_peek_by_id(loop->heap, id))
                {
                    heap_remove(loop->heap, id);
                }
                goto TIMER_CHECK;
            }
            else
            {
                // sleep for the difference in ms between now and the timer
                sleep = timer_subtract(&loop->heap->elements[0].timer, &current_time);
            }
        }
        else
        {
            sleep = -1;
        }
        if (loop->running) {
            epoll_return = epoll_wait(loop->epoll_fd, &event, 1, sleep);
        }
        else {
            epoll_return = 0;
        }

        // Call the callback for any signals that arrived
        for (int i = 0; i < _NSIG; i++)
        {
            if (signals_received[i]) 
            {
                signals_received[i] = false;
                cb_data_t *signal_cb_data = hash_get(loop->sig_map, i);
                signal_callback_f signal_cb = (signal_callback_f)signal_cb_data->cb;
                if (signal_cb_data != NULL)
                {
                    signal_cb(loop, i, signal_cb_data->data);
                }
            }
        }
        if (epoll_return == 1) {
            // Call the callback for the event that arrived
            poll_item_t *poll_item = event.data.ptr;
            event_e cb_event;
            if (event.events & EPOLLERR)
            {
                cb_event = ERROR_EVENT;
            }
            else if (event.events & EPOLLIN)
            {
                cb_event = READ_EVENT;
            }
            else if (event.events & EPOLLOUT)
            {
                cb_event = WRITE_EVENT;
            }
            else {
                DEBUG_PRINTF("Invalid event");
                cb_event = ERROR_EVENT;
            }
            poll_item->cb(loop, cb_event, poll_item->fd, poll_item->data);
        }
    }

    // Restore original signal handlers
    for (khint_t i = 0; i != kh_end(loop->sig_map); i++)
    {
		if (!kh_exist(loop->sig_map, i)) {
            continue;
        }
        int signum = kh_key(loop->sig_map, i);
        if (sigaction(signum, &oldacts[signum], NULL) == -1)
        {
            return -1;
        }
    }
    return 0;
}

void loop_fini(loop_t *loop)
{
    DEBUG_PRINTF("Cleaning up our loop!");
    int key;
    size_t id;
    void *value;
    kh_foreach(loop->fd_map, key, value, {
        free(value);
    })
    kh_foreach(loop->sig_map, key, value, {
        free(value);
    })
    kh_foreach(loop->timer_map, id, value, {
        free(value);
    })
    kh_foreach(loop->paused_timer_map, id, value, {
        free(value);
    })
    kh_destroy_map(loop->fd_map);
    kh_destroy_map(loop->sig_map);
    kh_destroy_sz_map(loop->timer_map);
    kh_destroy_sz_map(loop->paused_timer_map);
    heap_free(loop->heap);
    queue_free(loop->pending_fd_callbacks);
    close(loop->epoll_fd);
}
