#include <endian.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "loop.h"
#include "contexts.h"
#include "debug.h"
#include "hash.h"
#include "chess.h"
#include "vector.h"
#include "security.h"
#include "message_handler.h"

bool add_subscription(request_ctx_t *ctx, kh_str_map_t *hashtable, uint8_t *token) {
    char *str_token = session_token_to_str(token);
    bson_oid_t *oid = str_hash_get(server_ctx.session_tokens, str_token);
    if (oid == NULL) {
        queue_error(ctx, INVALID_SESSION_TOKEN);
        return false;
    }
    char *sid = malloc(SID_SIZE);
    bson_oid_to_string(oid, sid);
    int_vector_t *vector = str_hash_get(hashtable, sid);
    if (vector == NULL) {
        vector = int_vector_new(1);
        str_hash_add(hashtable, sid, vector);
    }
    int_vector_append(vector, ctx->connection_ctx->fd);
    return true;
}

void not_implemented_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    // Craft a payload, modify the info in connection context as needed
    DEBUG_PRINTF("Received valid packet that isn't yet supported");
    queue_error(ctx, NOT_IMPLEMENTED_ERROR);
    free(ctx);
}

void invalid_packet_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    DEBUG_PRINTF("Received a packet that isn't supported");
    queue_error(ctx, INVALID_PACKET_ERROR);
    ctx->connection_ctx->close_connection = true;
    free(ctx);
}

void read_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx)
{
    // Check if we've recieved all the bytes for the message
    // If we have not, read the bytes we do have into memory and store it somehow
    // Then we do all our other shit when we have all our bytes to eat
    if (event == ERROR_EVENT) {
        #ifndef NDEBUG
        int       error = 0;
        socklen_t errlen = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
        {
            DEBUG_PRINTF("error = %s\n", strerror(error));
        }
        #endif
        goto clean_up;
    }
    uint32_t message_length;
    uint32_t bytes_to_read;
    if (connection_ctx->total_bytes_read < 4) {
        bytes_to_read = 4 - connection_ctx->total_bytes_read;
    }
    else {
        message_length = be32toh(*(uint32_t*)connection_ctx->read_buffer);
        bytes_to_read = message_length - (connection_ctx->total_bytes_read - 4);
    }
    ssize_t bytes_read = read(
        fd,
        connection_ctx->read_buffer + connection_ctx->total_bytes_read, 
        bytes_to_read
    );
    if (bytes_read <= 0) {
        goto clean_up;
    }
    connection_ctx->total_bytes_read += bytes_read;
    if (connection_ctx->total_bytes_read < sizeof(uint32_t))
    {
        // Still waiting on the first 4 bytes to give us the message size
        return;
    }
    message_length = be32toh(*(uint32_t*)connection_ctx->read_buffer);
    if (connection_ctx->total_bytes_read < message_length + 4) {
        return;
    }
    message_handler(loop, fd, connection_ctx);
    return;
  clean_up:
    DEBUG_PRINTF("Read error encountered on fd %i", fd);
    loop_remove_fd(loop, fd);
    hash_remove(server_ctx.connection_contexts, fd);
    free(connection_ctx->read_buffer);
    free(connection_ctx->write_buffer);
    free(connection_ctx);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx) {
    if (event == ERROR_EVENT) {
        #ifndef NDEBUG
        int       error = 0;
        socklen_t errlen = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
        {
            DEBUG_PRINTF("error = %s\n", strerror(error));
        }
        #endif
        goto clean_up;
    }
    ssize_t bytes_written = write(fd, connection_ctx->write_buffer, connection_ctx->bytes_to_write);
    if (bytes_written == -1) {
        goto clean_up;
    }
    connection_ctx->bytes_to_write -= bytes_written;
    if (connection_ctx->bytes_to_write != 0) {
        memmove(
            connection_ctx->write_buffer, 
            connection_ctx->write_buffer + bytes_written,
            connection_ctx->bytes_to_write
        );
    }
    else if (connection_ctx->close_connection) {
        goto close_connection;
    }
    else
    {
        loop_add_fd(loop, fd, READ_EVENT, (fd_callback_f)read_handler, connection_ctx);
    }
    return;
  clean_up:
    DEBUG_PRINTF("Write error encountered on fd %i", fd);
  close_connection:
    loop_remove_fd(loop, fd);
    hash_remove(server_ctx.connection_contexts, fd);
    free(connection_ctx->read_buffer);
    free(connection_ctx->write_buffer);
    free(connection_ctx);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void read_write_handler(loop_t *loop, event_e event, int fd, connection_ctx_t *connection_ctx) {
    if (event == ERROR_EVENT) {
        #ifndef NDEBUG
        int       error = 0;
        socklen_t errlen = sizeof(error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
        {
            DEBUG_PRINTF("error = %s\n", strerror(error));
        }
        #endif
        goto clean_up;
    }
    else if (event == READ_EVENT) {
        read_handler(loop, event, fd, connection_ctx);
    }
    else if (event == WRITE_EVENT) {
        write_handler(loop, event, fd, connection_ctx);
    }
    return;
  clean_up:
    DEBUG_PRINTF("Error encountered on fd %i", fd);
    loop_remove_fd(loop, fd);
    hash_remove(server_ctx.connection_contexts, fd);
    free(connection_ctx->read_buffer);
    free(connection_ctx->write_buffer);
    free(connection_ctx);
    shutdown(fd, SHUT_RDWR);
    close(fd);
}
