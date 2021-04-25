#include "hash.h"

void hash_add(kh_map_t *hashtable, int key, void *value)
{
    int ret;
    khint_t i = kh_put_map(hashtable, key, &ret);
    kh_val(hashtable, i) = value;
}

void *hash_remove(kh_map_t *hashtable, int key)
{
    khint_t i = kh_get_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    void *value = kh_val(hashtable, i);
    kh_del_map(hashtable, i);
    return value;
}

void *hash_get(kh_map_t *hashtable, int key)
{
    khint_t i = kh_get_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    void *value = kh_val(hashtable, i);
    return value;
}

bool hash_contains(kh_map_t *hashtable, int key)
{
    khint_t i = kh_get_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    return kh_exist(hashtable, i) == 1;
}