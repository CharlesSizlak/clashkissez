#include "contexts.h"
#include "chess.h"
#include "handlers.h"
#include "debug.h"

void queue_write(request_ctx_t *ctx, uint8_t *buffer, size_t buffer_size) {
    connection_ctx_queue_write(ctx->connection_ctx, buffer, buffer_size);
}

void connection_ctx_queue_write(connection_ctx_t *ctx, uint8_t *buffer, size_t buffer_size) {
    if (ctx->bytes_to_write + buffer_size > ctx->write_buffer_capacity) {
        do {
            ctx->write_buffer_capacity *= 2;
        } while (ctx->bytes_to_write + buffer_size > ctx->write_buffer_capacity);
        ctx->write_buffer = realloc(ctx->write_buffer, ctx->write_buffer_capacity);
    }
    memcpy(ctx->write_buffer + ctx->bytes_to_write, buffer, buffer_size);
    ctx->bytes_to_write += buffer_size;
    loop_add_fd(ctx->loop, ctx->fd, READ_WRITE_EVENT, (fd_callback_f)read_write_handler, ctx);
}

void queue_error(request_ctx_t *ctx, Error_e error) {
    connection_ctx_queue_error(ctx->connection_ctx, error);
}

void connection_ctx_queue_error(connection_ctx_t *ctx, Error_e error) {
    DEBUG_PRINTF("Queueing up an error to send");
    ErrorReply_t *e = ErrorReply_new();
    ErrorReply_set_error(e, error);
    size_t buffer_size;
    uint8_t *buffer;
    ErrorReply_serialize(e, &buffer, &buffer_size);
    ErrorReply_free(e);
    connection_ctx_queue_write(ctx, buffer, buffer_size);
    free(buffer);
}
