#include "random.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/random.h>

#ifndef USE_SYS_RANDOM
#define USE_SYS_RANDOM 1
#endif

#if USE_SYS_RANDOM

void rand_init(void) {
}

void rand_get_bytes(void *buffer, size_t buffer_length) {
    size_t bytes_generated = 0;
    while (bytes_generated < buffer_length) {
        ssize_t last_bytes_generated = getrandom((uint8_t *)buffer + bytes_generated, buffer_length - bytes_generated, 0);
        if (last_bytes_generated == -1) {
            printf("getrandom failed, aborting program\n");
            abort();
        }
        bytes_generated += (size_t)last_bytes_generated;
    }
}

void rand_cleanup(void) {
}

#else
static int fd;

void rand_init(void) {
    fd = open("/dev/urandom", O_RDONLY);
}

void rand_get_bytes(void *buffer, size_t buffer_length) {
    size_t bytes_read = 0;
    while (bytes_read < buffer_length) {
        ssize_t last_read = read(fd, (uint8_t *)buffer + bytes_read, buffer_length - bytes_read);
        if (last_read <= 0) {
            printf("/dev/urandom failed, aborting program\n");
            abort();
        }
        bytes_read += (size_t)last_read;
    }
}

void rand_cleanup(void) {
    close(fd);
}
#endif
