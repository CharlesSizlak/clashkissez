#include <stdbool.h>
#include <mongoc/mongoc.h>

#define APP_NAME    "clashkissez"

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 1)
    {
        fprintf(stderr, "error: zero or two arguments required\n");
        return 1;
    }

    mongoc_init();

    mongoc_client_t *client = mongoc_client_new("mongodb://localhost:27017");
    mongoc_client_set_appname(client, APP_NAME);

    mongoc_database_t *database = mongoc_client_get_database(client, APP_NAME);
    mongoc_collection_t *collection = mongoc_client_get_collection(client, APP_NAME, "users");

    char *s;
    bson_t reply;
    bson_error_t error;
    bool status;

    bson_t *command = BCON_NEW("ping", BCON_INT32(1));
    status = mongoc_client_command_simple(
        client, "admin", command, NULL, &reply, &error);
    bson_destroy(command);

    s = bson_as_json(&reply, NULL);
    printf("%s\n", s);
    bson_free(s);
    bson_destroy(&reply);

    if (!status) {
        fprintf(stderr, "%s\n", error.message);
        return 1;
    }

    /* Insert to database
    if (argc != 1)
    {
        bson_oid_t oid;
        bson_oid_init(&oid, NULL);

        bson_t *insert = bson_new();
        BSON_APPEND_OID(insert, "_id", &oid);
        BSON_APPEND_UTF8(insert, argv[1], argv[2]);
        if (!mongoc_collection_insert_one(collection, insert, NULL, NULL, &error)) {
            fprintf(stderr, "%s\n", error.message);
        }
        bson_destroy(insert);
    }
    */

    /* Query
    bson_t *query = bson_new();
    bson_t *sub_document = bson_new();
    BSON_APPEND_BOOL(sub_document, "$exists", true);
    BSON_APPEND_DOCUMENT(query, "chicken", sub_document);
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts (collection, query, NULL, NULL);
    const bson_t *cdoc;

    {
        "chicken": {
            "$exists": true
        }
    }

    while (mongoc_cursor_next (cursor, &cdoc)) {
        s = bson_as_canonical_extended_json (cdoc, NULL);
        printf ("%s\n", s);
        bson_free (s);
    }
    */

    /* Update
     */

    /* Delete
     */

    bson_destroy (query);
    mongoc_cursor_destroy (cursor);
    mongoc_collection_destroy(collection);
    mongoc_database_destroy(database);
    mongoc_client_destroy(client);
    mongoc_cleanup();
    return 0;
}
