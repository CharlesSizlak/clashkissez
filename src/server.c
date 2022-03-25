#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <mongoc/mongoc.h>
#include "queue.h"
#include "hash.h"
#include "chess.h"
#include "loop.h"
#include "debug.h"
#include "random.h"
#include "vector.h"
#include "security.h"
#include "handlers.h"
#include "contexts.h"
#include "database.h"
#include "models.h"

server_ctx_t server_ctx;

static void sigint_handler(loop_t *loop, int signal, void *data) {
    loop->running = false;
}


static void ignore_handler(loop_t *loop, int signal, void *data) {
    // Do nothing
}

void accept_connection_cb(loop_t *loop, event_e event, int fd, void *data)
{
    // sit on the socket accepting new connections
    struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
    int conn_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
    if (conn_fd == -1) {
        printf("Failed to accept connection bruv.\n");
        return;
    }
    DEBUG_PRINTF("Connection accepted!");
    connection_ctx_t *connection_ctx = malloc(sizeof(connection_ctx_t));
    connection_ctx->total_bytes_read = 0;
    connection_ctx->read_buffer = malloc(4096);
    connection_ctx->read_buffer_capacity = 4096;
    connection_ctx->write_buffer = malloc(4096);
    connection_ctx->write_buffer_capacity = 4096;
    connection_ctx->bytes_to_write = 0;
    connection_ctx->fd = conn_fd;
    connection_ctx->close_connection = false;
    connection_ctx->loop = loop;
    connection_ctx->oid = NULL;
    hash_add(server_ctx.connection_contexts, conn_fd, connection_ctx);
    loop_add_fd(loop, conn_fd, READ_EVENT, (fd_callback_f)read_handler, connection_ctx);
    // In the instance we accept a connection, check their message against
    // a table of legal incoming messages

    // In the instance it's illegal, close the socket

    // In the instance we're legal and good to go, spin up a new fd
    // to handle this connection (which we already have) and put that
    // mfer in the event loop so he can handle the ongoing convo
}

int main(int argc, char **argv)
{
    // Don't buffer output streams
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    uint16_t PORT = 15873;

    // Initialize random
    rand_init();

    // prepare ourselves for recieving stuff by opening a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Failed to open a socket, see ya fucker.\n");
        return 1;
    }

#ifndef NDEBUG
    // Prevent waiting for re-bind in debug mode
    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
#endif


    // bind to the socket
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    int err = bind(server_fd,
	    (struct sockaddr*)&server_addr,
	    sizeof(server_addr));
    if (err == -1) {
        printf("Failed to bind socket. Don't ask me how.\n");
        return 1;
    }

    // listen on the socket
    err = listen(server_fd, 32);
    if (err == -1) {
        printf("Failed to listen on bound socket. How even??");
        return 1;
    }

    loop_t loop_storage;
    loop_t *loop = &loop_storage;
    server_ctx.connection_contexts = kh_init_map();
    server_ctx.session_tokens = kh_init_str_map();
    server_ctx.database_queue = queue_new();
    server_ctx.game_invite_subscriptions = kh_init_str_map();
    server_ctx.game_invite_response_subscriptions = kh_init_str_map();
    server_ctx.game_subscriptions = kh_init_str_map();
    server_ctx.friend_request_subscriptions = kh_init_str_map();
    server_ctx.friend_request_accepted_subscriptions = kh_init_str_map();
    server_ctx.active_games = kh_init_str_map();
    server_ctx.board_state_template = bson_new();
    for (size_t i = 0; i < BOARD_SIZE; i++)
    {
        char str_i[3];
        sprintf(str_i, "%zu", i);
        size_t row = ROW(i);
        size_t column = COLUMN(i);
        uint8_t piece = PIECE_NONE;
        if (row < 2 || row >= 6) {
            if (row == 1) {
                piece = PIECE_WHITE_PAWN;
            }
            else if (row == 6) {
                piece = PIECE_BLACK_PAWN;
            }
            else {
                if (column == 0 || column == 7) {
                    piece = PIECE_WHITE_ROOK;
                }
                else if (column == 1 || column == 6) {
                    piece = PIECE_WHITE_KNIGHT;
                }
                else if (column == 2 || column == 5) {
                    piece = PIECE_WHITE_BISHOP;
                }
                else if (column == 3) {
                    piece = PIECE_WHITE_QUEEN;
                }
                else if (column == 4) {
                    piece = PIECE_WHITE_KING;
                }
                if (row == 7) {
                    piece += 0x10;
                }
            }
        }
        BSON_APPEND_INT32(server_ctx.board_state_template, str_i, piece);
        server_ctx.memory_board_state_template[i] = piece;
    }
    

    loop_init(loop);
    
    loop_add_fd(loop, server_fd, READ_EVENT, accept_connection_cb, NULL);
    loop_add_signal(loop, SIGINT, sigint_handler, NULL);
    loop_add_signal(loop, SIGTERM, sigint_handler, NULL);
    loop_add_signal(loop, SIGUSR1, ignore_handler, NULL);

    // Spin up a thread to handle our database queries
    pthread_t database_thread_id;
    pthread_create(&database_thread_id, NULL, (thread_f)database_thread, loop);

    printf("Starting the server\n");
    loop_run(loop);
    printf("Server closing\n");
    queue_enqueue(server_ctx.database_queue, NULL);
    pthread_join(database_thread_id, NULL);
    loop_fini(loop);

    // TODO free leftovers in server_ctx.active_games
    int fd;
    connection_ctx_t *connection_ctx;
    kh_foreach(server_ctx.connection_contexts, fd, connection_ctx, {
        free(connection_ctx->read_buffer);
        free(connection_ctx->write_buffer);
        free(connection_ctx);
        shutdown(fd, SHUT_RDWR);
        close(fd);
    })
    kh_destroy_map(server_ctx.connection_contexts);
    const char *token;
    bson_oid_t *oid;
    kh_foreach(server_ctx.session_tokens, token, oid, {
        free(oid);
        free((char *)token);
    })
    kh_destroy_str_map(server_ctx.session_tokens);

    const char *sid;
    int_vector_t *vector;
    kh_foreach(server_ctx.game_invite_subscriptions, sid, vector, {
        free((char *)sid);
        int_vector_free(vector);
    })
    kh_destroy_str_map(server_ctx.game_invite_subscriptions);
    kh_foreach(server_ctx.game_invite_response_subscriptions, sid, vector, {
        free((char *)sid);
        int_vector_free(vector);
    })
    kh_destroy_str_map(server_ctx.game_invite_response_subscriptions);
    kh_foreach(server_ctx.game_subscriptions, sid, vector, {
        free((char *)sid);
        int_vector_free(vector);
    })
    kh_destroy_str_map(server_ctx.game_subscriptions);
    kh_foreach(server_ctx.friend_request_subscriptions, sid, vector, {
        free((char *)sid);
        int_vector_free(vector);
    })
    kh_destroy_str_map(server_ctx.friend_request_subscriptions);
    kh_foreach(server_ctx.friend_request_accepted_subscriptions, sid, vector, {
        free((char *)sid);
        int_vector_free(vector);
    })
    kh_destroy_str_map(server_ctx.friend_request_accepted_subscriptions);

    queue_free(server_ctx.database_queue);

    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    rand_cleanup();
    printf("Server closed\n");
}
