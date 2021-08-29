#include "random.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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