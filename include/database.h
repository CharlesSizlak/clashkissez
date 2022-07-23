#ifndef _DATABASE_H
#define _DATABASE_H

#include <mongoc/mongoc.h>
#include "contexts.h"
#include "loop.h"

/**
 * @brief Tracks the type of database operation to be performed.
 */
typedef enum database_operation_e {
    QUERY,
    UPDATE,
    INSERT,
    DELETE
} database_operation_e;

/**
 * @brief All data stored in the database will be under a member of database_collection_e.
 */
typedef enum database_collection_e {
    USERS,
    GAMES
} database_collection_e;

typedef void (*database_callback_f)(request_ctx_t *, void *);

/**
 * @brief Take the results from a database query and free any documents returned to us
 */
void results_free(bson_t **results);

/**
 * @brief Take the oid from a given document. Return should be freed.
 * 
 * @return A bson_oid_t that contains the specific identifier for the document
 */
bson_oid_t *get_oid(bson_t *obj);

/**
 * @brief Take the string representation of a document's oid. Return should be freed.
 * 
 * @return A string containing the OID of a given document.
 */
char *get_sid(bson_t *obj);

/**
 * @brief Queues a query for the database and passes the results to the provided callback.
 */
void database_query(request_ctx_t *ctx, database_collection_e collection, bson_t *query, database_callback_f cb);

/**
 * @brief Queues an insert operation for the database.
 */
void database_insert(request_ctx_t *ctx, database_collection_e collection, bson_t *insert, database_callback_f cb);

/**
 * @brief This is meant to be used in a thread that will handle database operations.
 */
void *database_thread(loop_t *loop);

#endif
