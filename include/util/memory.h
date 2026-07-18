/**
 * memory.h - Memory pool allocator for HL7DB
 *
 * Arena-based memory allocator for efficient allocation of many small objects.
 * Particularly useful for HL7 message parsing where we allocate many
 * segments, fields, and components that have the same lifetime.
 */

#ifndef HL7DB_MEMORY_H
#define HL7DB_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* Default pool size (64KB) */
#define MEMORY_POOL_DEFAULT_SIZE (64 * 1024)

/* Memory pool structure (opaque) */
typedef struct memory_pool memory_pool_t;

/**
 * Create a new memory pool
 *
 * @param size Initial size of the pool (0 = use default)
 * @return Pointer to new memory pool, or NULL on failure
 */
memory_pool_t *memory_pool_create(size_t size);

/**
 * Allocate memory from pool
 *
 * @param pool Memory pool to allocate from
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 *
 * Note: Memory is aligned to 8-byte boundaries
 */
void *memory_pool_alloc(memory_pool_t *pool, size_t size);

/**
 * Allocate and zero-initialize memory from pool
 *
 * @param pool Memory pool to allocate from
 * @param size Number of bytes to allocate
 * @return Pointer to allocated and zeroed memory, or NULL on failure
 */
void *memory_pool_calloc(memory_pool_t *pool, size_t size);

/**
 * Allocate memory for array from pool
 *
 * @param pool Memory pool to allocate from
 * @param nmemb Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory, or NULL on failure
 */
void *memory_pool_alloc_array(memory_pool_t *pool, size_t nmemb, size_t size);

/**
 * Duplicate a string in the pool
 *
 * @param pool Memory pool to allocate from
 * @param str String to duplicate
 * @return Pointer to duplicated string, or NULL on failure
 */
char *memory_pool_strdup(memory_pool_t *pool, const char *str);

/**
 * Duplicate a string with maximum length in the pool
 *
 * @param pool Memory pool to allocate from
 * @param str String to duplicate
 * @param n Maximum length to copy
 * @return Pointer to duplicated string, or NULL on failure
 */
char *memory_pool_strndup(memory_pool_t *pool, const char *str, size_t n);

/**
 * Reset pool to initial state (frees all allocations)
 *
 * @param pool Memory pool to reset
 *
 * Note: All pointers allocated from this pool become invalid
 */
void memory_pool_reset(memory_pool_t *pool);

/**
 * Destroy memory pool and free all resources
 *
 * @param pool Memory pool to destroy
 */
void memory_pool_destroy(memory_pool_t *pool);

/**
 * Get total bytes allocated from pool
 *
 * @param pool Memory pool
 * @return Total bytes allocated
 */
size_t memory_pool_get_allocated(const memory_pool_t *pool);

/**
 * Get total capacity of pool
 *
 * @param pool Memory pool
 * @return Total capacity in bytes
 */
size_t memory_pool_get_capacity(const memory_pool_t *pool);

/**
 * Get statistics about pool usage
 *
 * @param pool Memory pool
 * @param allocated Pointer to store allocated bytes (can be NULL)
 * @param capacity Pointer to store capacity bytes (can be NULL)
 * @param num_blocks Pointer to store number of blocks (can be NULL)
 */
void memory_pool_get_stats(const memory_pool_t *pool,
                           size_t *allocated,
                           size_t *capacity,
                           size_t *num_blocks);

#endif /* HL7DB_MEMORY_H */
