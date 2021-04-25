#define _POSIX_C_SOURCE 199309L
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "loop.h"
#include "hash.h"

static bool signals_received[_NSIG];

void signal_caught(int signum) 
{
    signals_received[signum] = true;
}

void loop_init(loop_t *loop)
{
    loop->epoll_fd = epoll_create1(0);
    loop->fd_map = kh_init_map();
    loop->sig_map = kh_init_map();
    sigemptyset(&loop->sigset);
}

int loop_add_fd(loop_t *loop, int fd, event_e event,
        fd_callback_f cb)
{
    poll_item_t *poll_item = malloc(sizeof(poll_item_t));
    poll_item->cb = cb;
    poll_item->fd = fd;
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
    ev.data.ptr = poll_item;
    epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    hash_add(loop->fd_map, fd, poll_item);
}

void loop_remove_fd(loop_t *loop, int fd)
{
    poll_item_t *poll_item = hash_remove(loop->fd_map, fd);
    free(poll_item);
    epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

//TODO test things to make sure they actually exist

int loop_add_signal(loop_t *loop, int signum, signal_callback_f cb)
{
    sigaddset(&loop->sigset, signum);
    hash_add(loop->sig_map, signum, cb);
}

void loop_remove_signal(loop_t *loop, int signum)
{
    sigdelset(&loop->sigset, signum);
    hash_remove(loop->sig_map, signum);
}

int loop_run(loop_t *loop) 
{
    loop->running = true;
    struct epoll_event event;
    int epoll_return;
    struct sigaction oldacts[_NSIG];
    struct sigaction act = {
        .sa_handler = signal_caught
    };
    sigemptyset(&act.sa_mask);

    //TODO finish adding timers in our event loop

    // Setup signal handlers for every added signal
    for (int i = 0; i != kh_end(loop->sig_map); i++)
    {
		if (kh_exist(loop->sig_map, i)) {
            continue;
        }
        int signum = kh_key(loop->sig_map, i);
        if (sigaction(signum, &act, &oldacts[signum]) == -1)
        {
            return -1;
        }
    }

    // Call epoll_wait until loop->running becomes false
    while (loop->running)
    {
        epoll_return = epoll_wait(loop->epoll_fd, &event, 1, -1);

        // Call the callback for any signals that arrived
        for (int i = 0; i < _NSIG; i++)
        {
            if (signals_received[i]) 
            {
                signals_received[i] = false;
                signal_callback_f signal_cb = hash_get(loop->sig_map, i);
                if (signal_cb != NULL)
                {
                    signal_cb(loop, i);
                }
            }
        }
        if (epoll_return == 1) {
            // Call the callback for the event that arrived
            poll_item_t *poll_item = event.data.ptr;
            poll_item->cb(loop, event.events, poll_item->fd);
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
    int key;
    poll_item_t *value;
    kh_foreach(loop->fd_map, key, value, {
        free(value);
    })
    kh_destroy_map(loop->fd_map);
    kh_destroy_map(loop->sig_map);
    close(loop->epoll_fd);
}