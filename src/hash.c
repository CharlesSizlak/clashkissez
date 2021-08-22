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

void sz_hash_add(kh_sz_map_t *hashtable, size_t key, void *value)
{
    int ret;
    khint_t i = kh_put_sz_map(hashtable, key, &ret);
    kh_val(hashtable, i) = value;
}

void *sz_hash_remove(kh_sz_map_t *hashtable, size_t key)
{
    khint_t i = kh_get_sz_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    void *value = kh_val(hashtable, i);
    kh_del_sz_map(hashtable, i);
    return value;
}

void *sz_hash_get(kh_sz_map_t *hashtable, size_t key)
{
    khint_t i = kh_get_sz_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    void *value = kh_val(hashtable, i);
    return value;
}

bool sz_hash_contains(kh_sz_map_t *hashtable, size_t key)
{
    khint_t i = kh_get_sz_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    return kh_exist(hashtable, i) == 1;
}

void str_hash_add(kh_str_map_t *hashtable, const char *key, void *value)
{
    int ret;
    khint_t i = kh_put_str_map(hashtable, key, &ret);
    kh_val(hashtable, i) = value;
}

void *str_hash_remove(kh_str_map_t *hashtable, const char *key)
{
    khint_t i = kh_get_str_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    void *value = kh_val(hashtable, i);
    kh_del_str_map(hashtable, i);
    return value;
}

void *str_hash_get(kh_str_map_t *hashtable, const char *key)
{
    khint_t i = kh_get_str_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    void *value = kh_val(hashtable, i);
    return value;
}

bool str_hash_contains(kh_str_map_t *hashtable, const char *key)
{
    khint_t i = kh_get_str_map(hashtable, key);
    if (i == kh_end(hashtable))
    {
        return NULL;
    }
    return kh_exist(hashtable, i) == 1;
}