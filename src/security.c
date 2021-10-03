#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "security.h"
#include "random.h"

bool safe_compare(const void *a, const void *b, size_t length) {
    int result = 0;
    for (size_t i=0; i < length; i++) {
        result |= ((const uint8_t *)a)[i] ^ ((const uint8_t *)b)[i];
    }
    return result == 0;
}

uint8_t *generate_session_token(void) {
    uint8_t *session_token = malloc(TOKEN_SIZE);
    rand_get_bytes(session_token, TOKEN_SIZE);
    return session_token;
}

char *generate_session_token_str(void) {
    uint8_t session_token[TOKEN_SIZE];
    rand_get_bytes(session_token, TOKEN_SIZE);
    char *str = malloc((TOKEN_SIZE * 2) + 1);
    for (size_t i = 0; i < TOKEN_SIZE; i++)
    {
        sprintf(str + (i * 2), "%02x", session_token[i]);
    }
    return str;
}

char *session_token_to_str(uint8_t *session_token) {
    char *str = malloc((TOKEN_SIZE * 2) + 1);
    for (size_t i = 0; i < TOKEN_SIZE; i++)
    {
        sprintf(str + (i * 2), "%02x", session_token[i]);
    }
    return str;
}
