/**
 * hashmap.h - Generic hash table for HL7DB
 *
 * Provides a hash table implementation for fast key-value lookups.
 * Useful for symbol tables, caches, and indexes.
 */

#ifndef HL7DB_HASHMAP_H
#define HL7DB_HASHMAP_H

#include <stddef.h>
#include <stdint.h>

/* Default initial capacity */
#define HASHMAP_DEFAULT_CAPACITY 16

/* Load factor threshold for resizing (0.75) */
#define HASHMAP_LOAD_FACTOR 0.75

/* Hash map structure (opaque) */
typedef struct hashmap hashmap_t;

/* Hash map iterator */
typedef struct hashmap_iter {
    const hashmap_t *map;
    size_t bucket_index;
    void *entry;
} hashmap_iter_t;

/* Function pointer types */
typedef void (*hashmap_free_fn)(void *value);
typedef uint64_t (*hashmap_hash_fn)(const void *key, size_t key_len);
typedef int (*hashmap_compare_fn)(const void *key1, size_t len1,
                                   const void *key2, size_t len2);

/**
 * Create a new hash map
 *
 * @param capacity Initial capacity (0 = use default)
 * @return Pointer to new hash map, or NULL on failure
 */
hashmap_t *hashmap_create(size_t capacity);

/**
 * Destroy hash map and optionally free values
 *
 * @param map Hash map to destroy
 * @param free_fn Function to free values (NULL = don't free values)
 *
 * Note: Keys are never freed (assumed to be managed externally or string literals)
 */
void hashmap_destroy(hashmap_t *map, hashmap_free_fn free_fn);

/**
 * Get number of entries in map
 *
 * @param map Hash map
 * @return Number of entries
 */
size_t hashmap_size(const hashmap_t *map);

/**
 * Check if map is empty
 *
 * @param map Hash map
 * @return 1 if empty, 0 otherwise
 */
int hashmap_is_empty(const hashmap_t *map);

/**
 * Put key-value pair into map (string key)
 *
 * @param map Hash map
 * @param key String key
 * @param value Value to store
 * @return Previous value if key existed, NULL otherwise
 *
 * Note: Key is not copied - caller must ensure it remains valid
 */
void *hashmap_put(hashmap_t *map, const char *key, void *value);

/**
 * Put key-value pair into map (binary key)
 *
 * @param map Hash map
 * @param key Binary key data
 * @param key_len Length of key in bytes
 * @param value Value to store
 * @return Previous value if key existed, NULL otherwise
 *
 * Note: Key is not copied - caller must ensure it remains valid
 */
void *hashmap_put_bin(hashmap_t *map, const void *key, size_t key_len, void *value);

/**
 * Get value for key (string key)
 *
 * @param map Hash map
 * @param key String key
 * @return Value for key, or NULL if not found
 */
void *hashmap_get(const hashmap_t *map, const char *key);

/**
 * Get value for key (binary key)
 *
 * @param map Hash map
 * @param key Binary key data
 * @param key_len Length of key in bytes
 * @return Value for key, or NULL if not found
 */
void *hashmap_get_bin(const hashmap_t *map, const void *key, size_t key_len);

/**
 * Check if map contains key (string key)
 *
 * @param map Hash map
 * @param key String key
 * @return 1 if key exists, 0 otherwise
 */
int hashmap_contains(const hashmap_t *map, const char *key);

/**
 * Check if map contains key (binary key)
 *
 * @param map Hash map
 * @param key Binary key data
 * @param key_len Length of key in bytes
 * @return 1 if key exists, 0 otherwise
 */
int hashmap_contains_bin(const hashmap_t *map, const void *key, size_t key_len);

/**
 * Remove entry from map (string key)
 *
 * @param map Hash map
 * @param key String key
 * @return Value that was removed, or NULL if not found
 */
void *hashmap_remove(hashmap_t *map, const char *key);

/**
 * Remove entry from map (binary key)
 *
 * @param map Hash map
 * @param key Binary key data
 * @param key_len Length of key in bytes
 * @return Value that was removed, or NULL if not found
 */
void *hashmap_remove_bin(hashmap_t *map, const void *key, size_t key_len);

/**
 * Clear all entries from map
 *
 * @param map Hash map
 * @param free_fn Function to free values (NULL = don't free values)
 */
void hashmap_clear(hashmap_t *map, hashmap_free_fn free_fn);

/**
 * Get iterator for map
 *
 * @param map Hash map
 * @param iter Iterator structure to initialize
 * @return 1 if iterator is valid, 0 if map is empty
 */
int hashmap_iter_init(const hashmap_t *map, hashmap_iter_t *iter);

/**
 * Move iterator to next entry
 *
 * @param iter Iterator
 * @return 1 if advanced to next entry, 0 if no more entries
 */
int hashmap_iter_next(hashmap_iter_t *iter);

/**
 * Get key from current iterator position
 *
 * @param iter Iterator
 * @param key_len Pointer to store key length (can be NULL)
 * @return Pointer to key, or NULL if iterator invalid
 */
const void *hashmap_iter_get_key(const hashmap_iter_t *iter, size_t *key_len);

/**
 * Get value from current iterator position
 *
 * @param iter Iterator
 * @return Pointer to value, or NULL if iterator invalid
 */
void *hashmap_iter_get_value(const hashmap_iter_t *iter);

/**
 * FNV-1a hash function (default)
 *
 * @param key Key data
 * @param len Length of key
 * @return Hash value
 */
uint64_t hashmap_hash_fnv1a(const void *key, size_t len);

#endif /* HL7DB_HASHMAP_H */
