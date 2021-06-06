#include "loop.h"
#include "time_heap.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

static void timer_print(loop_t * loop, size_t id, void *data)
{
    printf("Hey a timer went off with id %zu and data %p\n", id, data);
}

static void stdin_print(loop_t *loop, event_e event, int fd, void *data)
{
    printf("From stdin: ");
    char buf[4096];
    while (true)
    {
        ssize_t bytes = read(fd, buf, 4096);
        if (bytes == -1)
        {
            break;
        }
        buf[bytes] = '\0';
        printf("%s", buf);
    }
    printf("\n");
}

static void sigint_caught(loop_t *loop, int signal, void *data)
{
    if (signal == SIGINT)
    {
        printf("Caught a sigint, ending loop.\n");
        loop->running = false;
    }
    else
    {
        printf("We caught a signal #%i, not sure what he's doing here\n", signal);
    }
}

int main()
{
    // TODO add timers to a loop
    // timers call printf and print a time when they go off, tada
    // Add SIGUSR1(? the hell is this)
    // Add STDIN as a file descriptor and have it print off what we send along that fd

    // Instantiate a new instance of a loop and initialize it
    loop_t loop;
    loop_init(&loop);

    // Set up timers with random times to go off that are within 10 seconds of running
    // When those timers go off, print a message
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    printf("Adding timers...\n");
    for(int i = 3; i < 10; i+=3)
    {
        // Add a timer to the loop with the seconds interval increased by i
        struct timespec timer = {
            .tv_sec = current_time.tv_sec + i,
            .tv_nsec = current_time.tv_nsec
        };
        loop_add_timer(&loop, &timer, timer_print, (void*)(uintptr_t)i);
    }

    // Make STDIN work, print what we recieve along that 
    printf("Adding STDIN...\n");
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    loop_add_fd(&loop, STDIN_FILENO, READ_EVENT, stdin_print, NULL);


    // Catch SIGINT and print a message before shutting down with loop->running=false
    printf("Setting up to catch SIGINT...\n");
    loop_add_signal(&loop, SIGINT, sigint_caught, NULL);
    loop_add_signal(&loop, SIGUSR1, sigint_caught, NULL);

    loop_run(&loop);
    loop_fini(&loop);
}