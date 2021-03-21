#include <stdio.h>
#include "chess_builder.h"
#include "chess_reader.h"
#include "chess_verifier.h"

int main(int argc, char **arg)
{
    flatcc_builder_t builder, *B;
    B = &builder;
    flatcc_builder_init(B);
    //
    //flatbuffers_string_ref_t user = flatcc_builder_create_string(B, "bob", 3);
    //flatbuffers_uint8_vec_ref_t pass = flatbuffers_uint8_vec_create(B, "passw0rd", 8);
    //LoginRequest_create_as_root(B, user, pass);
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
    return 0;
}