#ifndef _DATABASE_H
#define _DATABASE_H

#include <mongoc/mongoc.h>
#include "contexts.h"
#include "loop.h"

typedef enum database_operation_e {
    QUERY,
    UPDATE,
    INSERT,
    DELETE
} database_operation_e;

typedef enum database_collection_e {
    USERS,
    GAMES
} database_collection_e;

typedef void (*database_callback_f)(request_ctx_t *, void *);

void results_free(bson_t **results);
bson_oid_t *get_oid(bson_t *obj);
char *get_sid(bson_t *obj);

void database_query(request_ctx_t *ctx, database_collection_e collection, bson_t *query, database_callback_f cb);
void database_insert(request_ctx_t *ctx, database_collection_e collection, bson_t *insert, database_callback_f cb);
void *database_thread(loop_t *loop);

#endif
