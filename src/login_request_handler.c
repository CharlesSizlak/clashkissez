#include <mongoc/mongoc.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdlib.h>
#include "contexts.h"
#include "database.h"
#include "chess.h"
#include "security.h"
#include "hash.h"
#include "handlers.h"
#include "debug.h"
#include "handler_utilities.h"

static void login_request_finish(request_ctx_t *ctx, bson_t **results);

void login_request_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    char *username;
    char *password;
    if (!LoginRequest_get_username(ctx->message, &username) ||
            !LoginRequest_get_password(ctx->message, &password)) {
        LoginRequest_free(ctx->message);
        invalid_packet_handler(loop, fd, ctx);
        return;
    }
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);
    database_query(ctx, USERS, query, (database_callback_f)login_request_finish);
}


/* TODO Create another hashtable that will map string versions of user oids
to a vector of active session tokens for that user. When a new session token is 
requested we add it to the vector and if the vector gets to an arbitrary size
then old session tokens get cleared out and removed as new ones come in. 
In addition, store a time stamp with each session token in the vector
for when that token expires and each time we validate a session token,
we check the time stamp to see if it's expired.
*/
static void login_request_finish(request_ctx_t *ctx, bson_t **results) {
    VALIDATE_QUERY_RESULTS(LoginRequest, LOGIN_FAILED_ERROR);
    uint8_t salt[SALT_SIZE];
    if (bson_has_field(results[0], "salt"))
    {
        bson_iter_t iterator;
        bson_iter_init(&iterator, results[0]);
        bson_iter_find(&iterator, "salt");
        const bson_value_t *value = bson_iter_value(&iterator);
        memcpy(salt, value->value.v_binary.data, SALT_SIZE);
    }
    else 
    {
        queue_error(ctx, INTERNAL_SERVER_ERROR);
        LoginRequest_free(ctx->message);
        results_free(results);
        free(ctx);
        return;
    }
    char *password;
    LoginRequest_get_password(ctx->message, &password);

    uint8_t hashed_password[HASH_SIZE];
    PKCS5_PBKDF2_HMAC_SHA1(
        password, 
        strlen(password), 
        salt, 
        SALT_SIZE, 
        ITERATION_COUNT, 
        HASH_SIZE, 
        hashed_password
    );

    uint8_t reference_password[HASH_SIZE];
    if (bson_has_field(results[0], "password"))
    {
        bson_iter_t iterator;
        bson_iter_init(&iterator, results[0]);
        bson_iter_find(&iterator, "password");
        const bson_value_t *value = bson_iter_value(&iterator);
        memcpy(reference_password, value->value.v_binary.data, HASH_SIZE);
    }
    else 
    {
        queue_error(ctx, INTERNAL_SERVER_ERROR);
        LoginRequest_free(ctx->message);
        results_free(results);
        free(ctx);
        return;
    }
    if (!safe_compare(hashed_password, reference_password, HASH_SIZE)) {
        DEBUG_PRINTF("Incorrect password");
        queue_error(ctx, LOGIN_FAILED_ERROR);
        LoginRequest_free(ctx->message);
        results_free(results);
        free(ctx);
        return;
    }

    uint8_t *session_token = generate_session_token();
    char *str_token = session_token_to_str(session_token);
    bson_oid_t *oid = get_oid(results[0]);
    str_hash_add(server_ctx.session_tokens, str_token, oid);
    LoginReply_t *r = LoginReply_new();
    LoginReply_set_session_token(r, session_token);
    free(session_token);
    size_t buffer_size;
    uint8_t *buffer;
    LoginReply_serialize(r, &buffer, &buffer_size);
    LoginReply_free(r);
    queue_write(ctx, buffer, buffer_size);
    free(buffer);
    results_free(results);
    LoginRequest_free(ctx->message);
    free(ctx);
}
