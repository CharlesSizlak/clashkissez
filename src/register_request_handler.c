#include <stdlib.h>
#include <mongoc/mongoc.h>
#include <openssl/evp.h>
#include "loop.h"
#include "contexts.h"
#include "chess.h"
#include "security.h"
#include "database.h"
#include "handlers.h"
#include "random.h"
#include "handler_utilities.h"

static void register_request_continue(request_ctx_t *ctx, bson_t **results);
static void register_request_finish(request_ctx_t *ctx, bson_t *result);


void register_request_handler(loop_t *loop, int fd, request_ctx_t *ctx) {
    // Pull out the username, check to see if it already exists in our database
    char *username;
    char *password;
    if (!RegisterRequest_get_username(ctx->message, &username) ||
            !RegisterRequest_get_password(ctx->message, &password)) {
        RegisterRequest_free(ctx->message);
        invalid_packet_handler(loop, fd, ctx);
        return;
    }
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);
    database_query(ctx, USERS, query, (database_callback_f)register_request_continue);
}

static void register_request_continue(request_ctx_t *ctx, bson_t **results) {
    // Check if there are any results. If there are results, pack up and send a register failed
    // error, then iterate over results and free the documents there.
    VALIDATE_QUERY_NO_RESULTS(RegisterRequest, REGISTER_FAILED_ERROR);
    char *username;
    char *password;
    RegisterRequest_get_username(ctx->message, &username);
    RegisterRequest_get_password(ctx->message, &password);

    // Generate a salt
    uint8_t salt[SALT_SIZE];
    rand_get_bytes(salt, SALT_SIZE);

    // Hash the password
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


    bson_t *insert = bson_new();
    BSON_APPEND_UTF8(insert, "username", username);
    BSON_APPEND_UTF8(insert, "debug_password", password);
    // Append the salt and the hashed password to the document
    BSON_APPEND_BINARY(insert, "password", BSON_SUBTYPE_BINARY, hashed_password, HASH_SIZE);
    BSON_APPEND_BINARY(insert, "salt", BSON_SUBTYPE_BINARY, salt, SALT_SIZE);

    bson_oid_init(&ctx->oid, NULL);
    BSON_APPEND_OID(insert, "_id", &ctx->oid);
    database_insert(ctx, USERS, insert, (database_callback_f)register_request_finish);
}

static void register_request_finish(request_ctx_t *ctx, bson_t *result) {
    if (result == NULL) {
        queue_error(ctx, DATABASE_ERROR);
        RegisterRequest_free(ctx->message);
        free(ctx);
        return;
    }
    int32_t insertedCount = 0;
    if (bson_has_field(result, "insertedCount"))
    {
        bson_iter_t iterator;
        bson_iter_init(&iterator, result);
        bson_iter_find(&iterator, "insertedCount");
        const bson_value_t *value = bson_iter_value(&iterator);
        insertedCount = value->value.v_int32;
    }
    if (insertedCount != 1) {
        char *username;
        char *password;
        RegisterRequest_get_username(ctx->message, &username);
        RegisterRequest_get_password(ctx->message, &password);
        bson_t *insert = bson_new();
        bson_t *array = bson_new();
        BSON_APPEND_UTF8(insert, "username", username);
        BSON_APPEND_UTF8(insert, "password", password);
        BSON_APPEND_ARRAY(insert, "games", array);
        bson_oid_init(&ctx->oid, NULL);
        BSON_APPEND_OID(insert, "_id", &ctx->oid);
        database_insert(ctx, USERS, insert, (database_callback_f)register_request_finish);
        return;
    }
    bson_oid_t *oid = malloc(sizeof(bson_oid_t));
    bson_oid_copy(&ctx->oid, oid);
    uint8_t *session_token = generate_session_token();
    char *str_token = session_token_to_str(session_token);
    str_hash_add(server_ctx.session_tokens, str_token, oid);
    RegisterReply_t *r = RegisterReply_new();
    RegisterReply_set_session_token(r, session_token);
    free(session_token);
    size_t buffer_size;
    uint8_t *buffer;
    RegisterReply_serialize(r, &buffer, &buffer_size);
    RegisterReply_free(r);
    queue_write(ctx, buffer, buffer_size);
    free(buffer);
    RegisterRequest_free(ctx->message);
    free(ctx);
}
