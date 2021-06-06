#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include "chess_builder.h"
#include "chess_reader.h"
#include "chess_verifier.h"
#include "loop.h"

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
    printf("Connection accepted!\n");
}


int main(int argc, char **arg)
{
    // TODO let the user set PORT from a command line later, defaulting to 15873
    uint16_t PORT = 15873;


    /* example of serialization for later
    flatcc_builder_t builder, *B;
    B = &builder;
    flatcc_builder_init(B);
    LoginRequest_start_as_root(B);
    LoginRequest_username_create_str(B, "bob");
    LoginRequest_password_create(B, "passw0rd", 8);
    LoginRequest_ref_t beep = LoginRequest_end_as_root(B);
    size_t buffer_size;
    uint8_t *buffer = flatcc_builder_finalize_aligned_buffer(B, &buffer_size);
    printf("Here's the stuff as it comes over the wire\n");
    for (size_t i = 0; i < buffer_size; i++) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    */

    // prepare ourselves for recieving stuff by opening a socket

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Failed to open a socket, see ya fucker.\n");
        return 1;
    }
    // bind to the socket
    // TODO at some point catch anything killing us that isn't sigkill, so we can clean up our trash

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

    loop_init(loop);
    
    //TODO remove null, replace with data
    loop_add_fd(loop, server_fd, READ_EVENT, accept_connection_cb, NULL);

    loop_run(loop);
    loop_fini(loop);

    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);


    /*

    list of file descriptors we're interested in
    file descriptors ready2go

    in our loop going brr one of our FD's is awake and ready to go
    check the event registered in FD's loop struct, check to see exactly what's going on
    run code ezpz

    if it's a new connection, we go ahead and try accepting that connection
    if we succeed, slap that in our event loop as an interested fd
    play with the ttl of the fd as wanted depending on what we feel like

    if it's an existing connection, we're going to check the incoming packet
    hash the serialized type of the packet and check it against a table of legal messages
    if it's legal, send the appropriate response
    keep the fd open on the instance we need to send a notif
    if it's illegal, close the connection and get rid of the fd from the event loop

    now for the rough part, doing chess things
    
    You know, we never have a moment where we're really doing a lot
    everything is either a response as a result of a packet coming in, or a response
    to our chess engine replying to us

    If it's our chess engine FD, we send out the appropriate notification if
    there's a fd asssociated with that game it's talking about

    TIMERS
    we have a list of games we're currently managing. We need to have ID's
    to pass around to the chess engine FD's to know what the f*!# they're talking about

    questions for ben
    A) can we make this the kernel's problem and have them YEET us a wakey
    in the instance that a timer hits 0

    B) ask ben about writing data into a file in a way that isn't terrible

    C) currently we don't plan to store draw offers anywhere, so 
    if a player misses the notification they just don't know it happened
    boy that seems annoying to solve if we want them to get it

    cursory exploration looks like we can in fact make this the kernel's problem

    timers we actually care about
    A) how long a connection fd lives before closing
    C) the timer associated with each players clock in a match

    */


    // read anything they send us over the wire
    // check to see if it's a valid loginrequest, if it is we send them a :)
    // if it's not we terminate the connection, politely
    /*
    printf("time for error\n");
    int status = LoginRequest_verify_as_root(buffer, buffer_size);
    if (status != 0) {
        printf("invalid buffer broski\n");
        printf("%s\n", flatcc_verify_error_string(status));
        return 1;
    }
    LoginRequest_table_t loginRequest = LoginRequest_as_root(buffer);
    const char *username = LoginRequest_username(loginRequest);
    const uint8_t *password = LoginRequest_password(loginRequest);
    size_t password_length = flatbuffers_uint8_vec_len(password);
    for (size_t i = 0; i < password_length; i++) {
        printf("%c", password[i]);
    }
    printf("\n");
    printf("%s\n", username);
    */
    
    return 0;
}