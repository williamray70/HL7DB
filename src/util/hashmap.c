/**
 * hashmap.c - Generic hash table implementation
 */

#include "util/hashmap.h"
#include <stdlib.h>
#include <string.h>

/* Hash map entry */
typedef struct hashmap_entry {
    const void *key;            /* Key (not owned by hashmap) */
    size_t key_len;             /* Length of key */
    void *value;                /* Value */
    uint64_t hash;              /* Cached hash value */
    struct hashmap_entry *next; /* Next entry in chain */
} hashmap_entry_t;

/* Hash map structure */
struct hashmap {
    hashmap_entry_t **buckets;  /* Array of bucket chains */
    size_t capacity;            /* Number of buckets */
    size_t size;                /* Number of entries */
    hashmap_hash_fn hash_fn;    /* Hash function */
    hashmap_compare_fn cmp_fn;  /* Key comparison function */
};

/**
 * Default key comparison (memcmp)
 */
static int default_key_compare(const void *key1, size_t len1,
                                const void *key2, size_t len2) {
    if (len1 != len2) {
        return (len1 < len2) ? -1 : 1;
    }
    return memcmp(key1, key2, len1);
}

/**
 * FNV-1a hash function
 */
uint64_t hashmap_hash_fnv1a(const void *key, size_t len) {
    const uint8_t *data = (const uint8_t *)key;
    uint64_t hash = 14695981039346656037ULL; /* FNV offset basis */

    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 1099511628211ULL; /* FNV prime */
    }

    return hash;
}

/**
 * Create a new hash map
 */
hashmap_t *hashmap_create(size_t capacity) {
    if (capacity == 0) {
        capacity = HASHMAP_DEFAULT_CAPACITY;
    }

    hashmap_t *map = malloc(sizeof(hashmap_t));
    if (!map) {
        return NULL;
    }

    map->buckets = calloc(capacity, sizeof(hashmap_entry_t *));
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    map->capacity = capacity;
    map->size = 0;
    map->hash_fn = hashmap_hash_fnv1a;
    map->cmp_fn = default_key_compare;

    return map;
}

/**
 * Resize hash map to new capacity
 */
static int hashmap_resize(hashmap_t *map, size_t new_capacity) {
    /* Allocate new bucket array */
    hashmap_entry_t **new_buckets = calloc(new_capacity, sizeof(hashmap_entry_t *));
    if (!new_buckets) {
        return -1;
    }

    /* Rehash all entries */
    for (size_t i = 0; i < map->capacity; i++) {
        hashmap_entry_t *entry = map->buckets[i];
        while (entry) {
            hashmap_entry_t *next = entry->next;

            /* Insert into new bucket array */
            size_t bucket = entry->hash % new_capacity;
            entry->next = new_buckets[bucket];
            new_buckets[bucket] = entry;

            entry = next;
        }
    }

    /* Replace old buckets */
    free(map->buckets);
    map->buckets = new_buckets;
    map->capacity = new_capacity;

    return 0;
}

/**
 * Destroy hash map and optionally free values
 */
void hashmap_destroy(hashmap_t *map, hashmap_free_fn free_fn) {
    if (!map) {
        return;
    }

    hashmap_clear(map, free_fn);
    free(map->buckets);
    free(map);
}

/**
 * Get number of entries in map
 */
size_t hashmap_size(const hashmap_t *map) {
    return map ? map->size : 0;
}

/**
 * Check if map is empty
 */
int hashmap_is_empty(const hashmap_t *map) {
    return map ? (map->size == 0) : 1;
}

/**
 * Put key-value pair into map (string key)
 */
void *hashmap_put(hashmap_t *map, const char *key, void *value) {
    return hashmap_put_bin(map, key, strlen(key), value);
}

/**
 * Put key-value pair into map (binary key)
 */
void *hashmap_put_bin(hashmap_t *map, const void *key, size_t key_len, void *value) {
    if (!map || !key) {
        return NULL;
    }

    /* Check if we need to resize */
    if ((double)map->size / map->capacity > HASHMAP_LOAD_FACTOR) {
        hashmap_resize(map, map->capacity * 2);
    }

    /* Calculate hash and bucket */
    uint64_t hash = map->hash_fn(key, key_len);
    size_t bucket = hash % map->capacity;

    /* Check if key already exists */
    hashmap_entry_t *entry = map->buckets[bucket];
    while (entry) {
        if (entry->hash == hash &&
            map->cmp_fn(entry->key, entry->key_len, key, key_len) == 0) {
            /* Key exists - replace value */
            void *old_value = entry->value;
            entry->value = value;
            return old_value;
        }
        entry = entry->next;
    }

    /* Create new entry */
    entry = malloc(sizeof(hashmap_entry_t));
    if (!entry) {
        return NULL;
    }

    entry->key = key;
    entry->key_len = key_len;
    entry->value = value;
    entry->hash = hash;
    entry->next = map->buckets[bucket];

    map->buckets[bucket] = entry;
    map->size++;

    return NULL;
}

/**
 * Get value for key (string key)
 */
void *hashmap_get(const hashmap_t *map, const char *key) {
    return hashmap_get_bin(map, key, strlen(key));
}

/**
 * Get value for key (binary key)
 */
void *hashmap_get_bin(const hashmap_t *map, const void *key, size_t key_len) {
    if (!map || !key) {
        return NULL;
    }

    /* Calculate hash and bucket */
    uint64_t hash = map->hash_fn(key, key_len);
    size_t bucket = hash % map->capacity;

    /* Search for entry */
    hashmap_entry_t *entry = map->buckets[bucket];
    while (entry) {
        if (entry->hash == hash &&
            map->cmp_fn(entry->key, entry->key_len, key, key_len) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL;
}

/**
 * Check if map contains key (string key)
 */
int hashmap_contains(const hashmap_t *map, const char *key) {
    return hashmap_contains_bin(map, key, strlen(key));
}

/**
 * Check if map contains key (binary key)
 */
int hashmap_contains_bin(const hashmap_t *map, const void *key, size_t key_len) {
    return hashmap_get_bin(map, key, key_len) != NULL;
}

/**
 * Remove entry from map (string key)
 */
void *hashmap_remove(hashmap_t *map, const char *key) {
    return hashmap_remove_bin(map, key, strlen(key));
}

/**
 * Remove entry from map (binary key)
 */
void *hashmap_remove_bin(hashmap_t *map, const void *key, size_t key_len) {
    if (!map || !key) {
        return NULL;
    }

    /* Calculate hash and bucket */
    uint64_t hash = map->hash_fn(key, key_len);
    size_t bucket = hash % map->capacity;

    /* Search for entry */
    hashmap_entry_t **entry_ptr = &map->buckets[bucket];
    while (*entry_ptr) {
        hashmap_entry_t *entry = *entry_ptr;

        if (entry->hash == hash &&
            map->cmp_fn(entry->key, entry->key_len, key, key_len) == 0) {
            /* Found - remove from chain */
            *entry_ptr = entry->next;

            void *value = entry->value;
            free(entry);
            map->size--;

            return value;
        }

        entry_ptr = &entry->next;
    }

    return NULL;
}

/**
 * Clear all entries from map
 */
void hashmap_clear(hashmap_t *map, hashmap_free_fn free_fn) {
    if (!map) {
        return;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        hashmap_entry_t *entry = map->buckets[i];
        while (entry) {
            hashmap_entry_t *next = entry->next;

            if (free_fn && entry->value) {
                free_fn(entry->value);
            }

            free(entry);
            entry = next;
        }
        map->buckets[i] = NULL;
    }

    map->size = 0;
}

/**
 * Get iterator for map
 */
int hashmap_iter_init(const hashmap_t *map, hashmap_iter_t *iter) {
    if (!map || !iter) {
        return 0;
    }

    iter->map = map;
    iter->bucket_index = 0;
    iter->entry = NULL;

    /* Find first non-empty bucket */
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->buckets[i]) {
            iter->bucket_index = i;
            iter->entry = map->buckets[i];
            return 1;
        }
    }

    return 0; /* Map is empty */
}

/**
 * Move iterator to next entry
 */
int hashmap_iter_next(hashmap_iter_t *iter) {
    if (!iter || !iter->map || !iter->entry) {
        return 0;
    }

    hashmap_entry_t *entry = (hashmap_entry_t *)iter->entry;

    /* Try next entry in current chain */
    if (entry->next) {
        iter->entry = entry->next;
        return 1;
    }

    /* Move to next non-empty bucket */
    for (size_t i = iter->bucket_index + 1; i < iter->map->capacity; i++) {
        if (iter->map->buckets[i]) {
            iter->bucket_index = i;
            iter->entry = iter->map->buckets[i];
            return 1;
        }
    }

    /* No more entries */
    iter->entry = NULL;
    return 0;
}

/**
 * Get key from current iterator position
 */
const void *hashmap_iter_get_key(const hashmap_iter_t *iter, size_t *key_len) {
    if (!iter || !iter->entry) {
        return NULL;
    }

    hashmap_entry_t *entry = (hashmap_entry_t *)iter->entry;
    if (key_len) {
        *key_len = entry->key_len;
    }

    return entry->key;
}

/**
 * Get value from current iterator position
 */
void *hashmap_iter_get_value(const hashmap_iter_t *iter) {
    if (!iter || !iter->entry) {
        return NULL;
    }

    hashmap_entry_t *entry = (hashmap_entry_t *)iter->entry;
    return entry->value;
}
