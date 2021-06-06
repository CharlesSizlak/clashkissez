#ifndef _HASH_H
#define _HASH_H

#include <khash.h>
#include <stdbool.h>

KHASH_MAP_INIT_INT(map, void*);
KHASH_MAP_INIT_INT(sz_map, void*);

void hash_add(kh_map_t *hashtable, int key, void *value);

void *hash_remove(kh_map_t *hashtable, int key);

void *hash_get(kh_map_t *hashtable, int key);

bool hash_contains(kh_map_t *hashtable, int key);

void sz_hash_add(kh_sz_map_t *hashtable, size_t key, void *value);

void *sz_hash_remove(kh_sz_map_t *hashtable, size_t key);

void *sz_hash_get(kh_sz_map_t *hashtable, size_t key);

bool sz_hash_contains(kh_sz_map_t *hashtable, size_t key);

#endif