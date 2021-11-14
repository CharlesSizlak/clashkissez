#ifndef _HANDLER_UTILITIES_H
#define _HANDLER_UTILITIES_H

#include "handlers.h"
#include "chess.h"
#include "database.h"
#include "hash.h"
#include "security.h"
#include <stdlib.h>

#define VALIDATE_SESSION_TOKEN(RequestType, Oid)                                                                  \
    do                                                                                                            \
    {                                                                                                             \
        if (ctx->connection_ctx->oid != NULL) \
        { \
            Oid = ctx->connection_ctx->oid; \
            break; \
        } \
        uint8_t *_validate_session_token;                                                                         \
        if (!RequestType##_get_session_token(ctx->message, &_validate_session_token))                             \
        {                                                                                                         \
            queue_error(ctx, INVALID_PACKET_ERROR);                                                               \
            RequestType##_free(ctx->message);                                                                     \
            free(ctx);                                                                                            \
            return;                                                                                               \
        }                                                                                                         \
        char *_validate_session_str_token = session_token_to_str(_validate_session_token);                        \
        bson_oid_t *_validate_session_oid = str_hash_get(server_ctx.session_tokens, _validate_session_str_token); \
        free(_validate_session_str_token);                                                                        \
        if (_validate_session_oid == NULL)                                                                        \
        {                                                                                                         \
            queue_error(ctx, INVALID_SESSION_TOKEN);                                                              \
            RequestType##_free(ctx->message);                                                                     \
            free(ctx);                                                                                            \
            return;                                                                                               \
        }                                                                                                         \
        Oid = _validate_session_oid;                                                                              \
        ctx->connection_ctx->oid = _validate_session_oid; \
    }                                                                                                             \
    while(0)

#define VALIDATE_QUERY_NO_RESULTS(RequestType, ErrorType) \
    do \
    { \
        if (results == NULL) { \
            queue_error(ctx, DATABASE_ERROR); \
            RequestType##_free(ctx->message); \
            free(ctx); \
            return; \
        } \
        if (results[0] != NULL) { \
            queue_error(ctx, ErrorType); \
            RequestType##_free(ctx->message); \
            results_free(results); \
            free(ctx); \
            return; \
        } \
    } \
    while(0)

#define VALIDATE_QUERY_RESULTS(RequestType, ErrorType) \
    do \
    { \
        if (results == NULL) { \
            queue_error(ctx, DATABASE_ERROR); \
            RequestType##_free(ctx->message); \
            free(ctx); \
            return; \
        } \
        if (results[0] == NULL) { \
            queue_error(ctx, ErrorType); \
            RequestType##_free(ctx->message); \
            results_free(results); \
            free(ctx); \
            return; \
        } \
    } \
    while(0)

#endif
