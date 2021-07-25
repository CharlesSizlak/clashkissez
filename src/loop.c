#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "loop.h"
#include "hash.h"
#include "time_heap.h"
#include "debug.h"

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

  /* Return 1 if result is negative.
  return x->tv_sec < y->tv_sec;
}
*/

/**
 * Returns the difference in milliseconds of a - b
 */
static int timer_subtract(struct timespec *a, struct timespec *b)
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
    sigemptyset(&loop->sigset);
    loop->heap = heap_create();
}

int loop_add_fd(loop_t *loop, int fd, event_e event,
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

//TODO test things to make sure they actually exist

void loop_add_signal(loop_t *loop, int signum, 
    signal_callback_f cb, void *data)
{
    sigaddset(&loop->sigset, signum);
    cb_data_t *cb_data = malloc(sizeof(cb_data_t));
    cb_data->data = data;
    cb_data->cb = cb;
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
    cb_data->cb = cb;
    sz_hash_add(loop->timer_map, id, cb_data);
    return id;
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

int loop_run(loop_t *loop) 
{
    DEBUG_PRINTF("Entering loop!");
    loop->running = true;
    struct epoll_event event;
    int epoll_return;
    size_t id;
    struct sigaction oldacts[_NSIG];
    struct sigaction act = {
        .sa_handler = signal_caught
    };
    sigemptyset(&act.sa_mask);
    int sleep = -1;

    // Setup signal handlers for every added signal
    for (int i = 0; i != kh_end(loop->sig_map); i++)
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
        DEBUG_PRINTF("In the while loop of the loop!");
        // Note for self, argument #4 is a int timer that blocks for
        // that many milliseconds, measured against CLOCK_MONOTONIC
        TIMER_CHECK:
        if (loop->heap->total_elements)
        {
            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            if (heap_peek(loop->heap))
            {
                // Pop off the top timer, call its cb
                // Then check the next timer and go until we're able to sleep
                id = loop->heap->elements[0].id;
                cb_data_t *timer_cb_data = sz_hash_get(loop->timer_map, id);
                timer_callback_f timer_cb = timer_cb_data->cb;
                timer_cb(loop, id, timer_cb_data->data);
                if (heap_peek(loop->heap))
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

        epoll_return = epoll_wait(loop->epoll_fd, &event, 1, sleep);

        // Call the callback for any signals that arrived
        for (int i = 0; i < _NSIG; i++)
        {
            if (signals_received[i]) 
            {
                signals_received[i] = false;
                cb_data_t *signal_cb_data = hash_get(loop->sig_map, i);
                signal_callback_f signal_cb = signal_cb_data->cb;
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
            poll_item->cb(loop, cb_event, poll_item->fd, poll_item->data);
        }
    }

    // Restore original signal handlers
    for (int i = 0; i != kh_end(loop->sig_map); i++)
    {
		if (kh_exist(loop->sig_map, i)) {
            continue;
        }
        int signum = kh_key(loop->sig_map, i);
        if (sigaction(signum, &oldacts[signum], NULL) == -1)
        {
            return -1;
        }
    }
}

void loop_fini(loop_t *loop)
{
    DEBUG_PRINTF("Cleaning up our loop!");
    int key;
    poll_item_t *value;
    kh_foreach(loop->fd_map, key, value, {
        free(value);
    })
    kh_destroy_map(loop->fd_map);
    kh_destroy_map(loop->sig_map);
    kh_destroy_sz_map(loop->timer_map);
    heap_free(loop->heap);
    close(loop->epoll_fd);
}