#include "loop.h"
#include <stdio.h>
#include <time.h>


/*
Take a timer that lasts 5 seconds, and a timer that lasts 2 and a half seconds
When the 2 and a half second timer goes off, pause the 5 second timer
Start a new timer after pausing for 5 seconds, which unpauses the other timer when it expires
Take snapshots of the time when the first timer started and when the first timer ends
Find the difference, it should be 12 and a half seconds
ez clap
*/

void print_current_time(void) {
    struct timespec current_time;
    struct tm result;
    clock_gettime(CLOCK_REALTIME, &current_time);
    localtime_r(&current_time.tv_sec, &result);
    double fractional_seconds = (double)current_time.tv_nsec / 1000000000.0;
    char buf[64];
    asctime_r(&result, buf);
    printf("Time: %sand %.1f seconds\n", buf, fractional_seconds);
}

void five_second_timer_cb(loop_t *loop, size_t id, void *data) {
    printf("Initial five second timer is up, ending loop\n");
    print_current_time();
    loop->running = false;
}

void other_five_second_timer_cb(loop_t *loop, size_t id, size_t *data) {
    loop_unpause_timer(loop, *data);
    printf("Five seconds have passed, unpausing initial timer\n");
    print_current_time();
}

void two_and_a_half_second_timer_cb(loop_t *loop, size_t id, size_t *data) {
    loop_pause_timer(loop, *data);
    struct timespec five_second_timer;
    clock_gettime(CLOCK_MONOTONIC, &five_second_timer);
    five_second_timer.tv_sec += 5;
    loop_add_timer(loop, &five_second_timer, (timer_callback_f)other_five_second_timer_cb, data);
    printf("Two and a half seconds are up\n");
    print_current_time();
}

int main(void) {
    loop_t loop;
    loop_init(&loop);
    printf("Starting 5 second timer\n");
    struct timespec five_second_timer;
    struct timespec two_and_a_half_second_timer;
    clock_gettime(CLOCK_MONOTONIC, &five_second_timer);
    clock_gettime(CLOCK_MONOTONIC, &two_and_a_half_second_timer);
    five_second_timer.tv_sec += 5;
    timer_add(&two_and_a_half_second_timer, 2500);
    printf("Initial time\n");
    print_current_time();
    size_t id = loop_add_timer (&loop, &five_second_timer, five_second_timer_cb, NULL);
    loop_add_timer(&loop, &two_and_a_half_second_timer, (timer_callback_f)two_and_a_half_second_timer_cb, &id);
    loop_run(&loop);
    loop_fini(&loop);
}
