#ifndef _HASH_H
#define _HASH_H

#include <khash.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "models.h"

#define board_compare(a, b) (memcmp(a, b, BOARD_SIZE) == 0)
#define board_hash(key) board_hash(key)

//TODO Finish implementing a hash for game boards

KHASH_MAP_INIT_INT(map, void*)
KHASH_MAP_INIT_INT(sz_map, void*)
KHASH_MAP_INIT_STR(str_map, void*)

// Hashtable operations that use ints as keys
void hash_add(kh_map_t *hashtable, int key, void *value);

void *hash_remove(kh_map_t *hashtable, int key);

void *hash_get(kh_map_t *hashtable, int key);

bool hash_contains(kh_map_t *hashtable, int key);

// Hashtable operations that use a size_t as a key
void sz_hash_add(kh_sz_map_t *hashtable, size_t key, void *value);

void *sz_hash_remove(kh_sz_map_t *hashtable, size_t key);

void *sz_hash_get(kh_sz_map_t *hashtable, size_t key);

bool sz_hash_contains(kh_sz_map_t *hashtable, size_t key);

// Hashtable operations that4 use strings as keys
void str_hash_add(kh_str_map_t *hashtable, const char *key, void *value);

void *str_hash_remove(kh_str_map_t *hashtable, const char *key);

void *str_hash_get(kh_str_map_t *hashtable, const char *key);

bool str_hash_contains(kh_str_map_t *hashtable, const char *key);

#endif
