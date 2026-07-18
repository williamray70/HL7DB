/**
 * memory.c - Memory pool allocator implementation
 */

#include "util/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/* Alignment for allocated memory (8 bytes) */
#define MEMORY_ALIGNMENT 8

/* Align size to MEMORY_ALIGNMENT boundary */
#define ALIGN_SIZE(size) (((size) + MEMORY_ALIGNMENT - 1) & ~(MEMORY_ALIGNMENT - 1))

/* Memory block structure */
typedef struct memory_block {
    uint8_t *data;              /* Block data */
    size_t size;                /* Total size of block */
    size_t used;                /* Bytes used in block */
    struct memory_block *next;  /* Next block in chain */
} memory_block_t;

/* Memory pool structure */
struct memory_pool {
    memory_block_t *blocks;     /* Linked list of blocks */
    memory_block_t *current;    /* Current block for allocation */
    size_t block_size;          /* Default size for new blocks */
    size_t total_allocated;     /* Total bytes allocated */
    size_t total_capacity;      /* Total capacity across all blocks */
    size_t num_blocks;          /* Number of blocks */
};

/**
 * Create a new memory block
 */
static memory_block_t *create_block(size_t size) {
    memory_block_t *block = malloc(sizeof(memory_block_t));
    if (!block) {
        return NULL;
    }

    block->data = malloc(size);
    if (!block->data) {
        free(block);
        return NULL;
    }

    block->size = size;
    block->used = 0;
    block->next = NULL;

    return block;
}

/**
 * Free a memory block
 */
static void free_block(memory_block_t *block) {
    if (block) {
        free(block->data);
        free(block);
    }
}

/**
 * Create a new memory pool
 */
memory_pool_t *memory_pool_create(size_t size) {
    if (size == 0) {
        size = MEMORY_POOL_DEFAULT_SIZE;
    }

    memory_pool_t *pool = malloc(sizeof(memory_pool_t));
    if (!pool) {
        return NULL;
    }

    /* Create initial block */
    pool->blocks = create_block(size);
    if (!pool->blocks) {
        free(pool);
        return NULL;
    }

    pool->current = pool->blocks;
    pool->block_size = size;
    pool->total_allocated = 0;
    pool->total_capacity = size;
    pool->num_blocks = 1;

    return pool;
}

/**
 * Allocate memory from pool
 */
void *memory_pool_alloc(memory_pool_t *pool, size_t size) {
    if (!pool || size == 0) {
        return NULL;
    }

    /* Align size */
    size_t aligned_size = ALIGN_SIZE(size);

    /* Check if current block has enough space */
    memory_block_t *block = pool->current;
    if (block->used + aligned_size > block->size) {
        /* Need a new block */
        size_t new_block_size = (aligned_size > pool->block_size)
                                ? aligned_size
                                : pool->block_size;

        memory_block_t *new_block = create_block(new_block_size);
        if (!new_block) {
            return NULL;
        }

        /* Add to end of block list */
        new_block->next = NULL;
        pool->current->next = new_block;
        pool->current = new_block;
        pool->total_capacity += new_block_size;
        pool->num_blocks++;

        block = new_block;
    }

    /* Allocate from current block */
    void *ptr = block->data + block->used;
    block->used += aligned_size;
    pool->total_allocated += aligned_size;

    return ptr;
}

/**
 * Allocate and zero-initialize memory from pool
 */
void *memory_pool_calloc(memory_pool_t *pool, size_t size) {
    void *ptr = memory_pool_alloc(pool, size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/**
 * Allocate memory for array from pool
 */
void *memory_pool_alloc_array(memory_pool_t *pool, size_t nmemb, size_t size) {
    /* Check for overflow */
    if (nmemb > 0 && size > SIZE_MAX / nmemb) {
        return NULL;
    }

    return memory_pool_alloc(pool, nmemb * size);
}

/**
 * Duplicate a string in the pool
 */
char *memory_pool_strdup(memory_pool_t *pool, const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *copy = memory_pool_alloc(pool, len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

/**
 * Duplicate a string with maximum length in the pool
 */
char *memory_pool_strndup(memory_pool_t *pool, const char *str, size_t n) {
    if (!str) {
        return NULL;
    }

    size_t len = 0;
    while (len < n && str[len] != '\0') {
        len++;
    }

    char *copy = memory_pool_alloc(pool, len + 1);
    if (copy) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

/**
 * Reset pool to initial state
 */
void memory_pool_reset(memory_pool_t *pool) {
    if (!pool) {
        return;
    }

    /* Free all blocks except the first one */
    memory_block_t *block = pool->blocks->next;
    while (block) {
        memory_block_t *next = block->next;
        free_block(block);
        block = next;
    }

    /* Reset first block */
    pool->blocks->used = 0;
    pool->blocks->next = NULL;

    /* Reset pool state */
    pool->current = pool->blocks;
    pool->total_allocated = 0;
    pool->total_capacity = pool->blocks->size;
    pool->num_blocks = 1;
}

/**
 * Destroy memory pool and free all resources
 */
void memory_pool_destroy(memory_pool_t *pool) {
    if (!pool) {
        return;
    }

    /* Free all blocks */
    memory_block_t *block = pool->blocks;
    while (block) {
        memory_block_t *next = block->next;
        free_block(block);
        block = next;
    }

    /* Free pool structure */
    free(pool);
}

/**
 * Get total bytes allocated from pool
 */
size_t memory_pool_get_allocated(const memory_pool_t *pool) {
    return pool ? pool->total_allocated : 0;
}

/**
 * Get total capacity of pool
 */
size_t memory_pool_get_capacity(const memory_pool_t *pool) {
    return pool ? pool->total_capacity : 0;
}

/**
 * Get statistics about pool usage
 */
void memory_pool_get_stats(const memory_pool_t *pool,
                           size_t *allocated,
                           size_t *capacity,
                           size_t *num_blocks) {
    if (!pool) {
        if (allocated) *allocated = 0;
        if (capacity) *capacity = 0;
        if (num_blocks) *num_blocks = 0;
        return;
    }

    if (allocated) {
        *allocated = pool->total_allocated;
    }
    if (capacity) {
        *capacity = pool->total_capacity;
    }
    if (num_blocks) {
        *num_blocks = pool->num_blocks;
    }
}
