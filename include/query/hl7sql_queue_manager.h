/**
 * hl7sql_queue_manager.h - HL7SQL Queue Manager
 *
 * Manages multiple named queues for HL7 message organization.
 * Each queue is an independent message store with its own lock.
 */

#ifndef HL7SQL_QUEUE_MANAGER_H
#define HL7SQL_QUEUE_MANAGER_H

#include "query/hl7sql_store.h"
#include "storage/persistence.h"
#include "util/hashmap.h"
#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * History configuration
 */
typedef struct {
    size_t max_history_size;           /* Max messages in history (0 = unlimited) */
    bool auto_rotate;                  /* Auto-remove oldest when limit reached */
} history_config_t;

/**
 * Queue manager structure
 *
 * Manages a collection of named queues, each containing a separate message store.
 * Thread-safe with read-write lock for queue creation/deletion.
 */
typedef struct {
    hashmap_t *queues;               /* Map: queue_name (char*) -> hl7sql_store_t* */
    hashmap_t *history_queues;       /* Map: queue_name (char*) -> hl7sql_store_t* (history) */
    hashmap_t *persistence_handles;    /* Map: queue_name (char*) -> queue_persistence_t* */
    hashmap_t *history_persistence_handles; /* Map: queue_name (char*) -> queue_persistence_t* (history) */
    pthread_rwlock_t lock;             /* RW lock for queue CRUD operations */
    persistence_config_t *persistence; /* Persistence configuration (NULL if disabled) */
    history_config_t history_config;   /* History settings */
} hl7sql_queue_manager_t;

/* ====================================================================
 * Lifecycle Management
 * ==================================================================== */

/**
 * Create a new queue manager
 *
 * Returns:
 *   Pointer to new queue manager, or NULL on failure
 */
hl7sql_queue_manager_t *hl7sql_queue_manager_create(void);

/**
 * Set persistence configuration for the queue manager
 *
 * Must be called before creating any queues.
 * If persistence is enabled, all future queue creations will persist messages,
 * and existing persisted queues will be loaded on startup.
 *
 * Args:
 *   mgr: Queue manager
 *   config: Persistence configuration (manager keeps reference, don't free)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int hl7sql_queue_manager_set_persistence(hl7sql_queue_manager_t *mgr,
                                             persistence_config_t *config);

/**
 * Load all queues from registry file
 *
 * Reads the channel registry and recreates all registered queues,
 * loading their persisted messages if available.
 * Should be called after set_persistence() to restore queues on startup.
 *
 * Args:
 *   mgr: Queue manager
 *
 * Returns:
 *   Number of queues loaded, or -1 on failure
 */
int hl7sql_queue_manager_load_registry(hl7sql_queue_manager_t *mgr);

/**
 * Set history configuration for the queue manager
 *
 * Configures history size limits and rotation behavior.
 *
 * Args:
 *   mgr: Queue manager
 *   max_size: Maximum messages in history (0 = unlimited)
 *   auto_rotate: Automatically remove oldest when limit reached
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int hl7sql_queue_manager_set_history_config(hl7sql_queue_manager_t *mgr,
                                                size_t max_size,
                                                bool auto_rotate);

/**
 * Destroy queue manager and all its queues
 *
 * Frees all stores and queue names. Queues need not be empty.
 *
 * Args:
 *   mgr: Queue manager to destroy (may be NULL)
 */
void hl7sql_queue_manager_destroy(hl7sql_queue_manager_t *mgr);

/* ====================================================================
 * Queue Operations
 * ==================================================================== */

/**
 * Create a new queue
 *
 * Creates an empty message store with the given name.
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name (must be unique, will be copied)
 *
 * Returns:
 *   0 on success, -1 if queue already exists, -2 on allocation failure
 */
int hl7sql_queue_manager_create_queue(hl7sql_queue_manager_t *mgr,
                                            const char *name);

/**
 * Drop (delete) an existing queue
 *
 * SAFETY: Queue must be empty before deletion.
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name to drop
 *
 * Returns:
 *   0 on success, -1 if not found, -2 if not empty
 */
int hl7sql_queue_manager_drop_queue(hl7sql_queue_manager_t *mgr,
                                          const char *name);

/**
 * Get a queue's message store
 *
 * Returns a pointer to the store for querying/inserting messages.
 * Caller must use store's own locks for thread safety.
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name
 *
 * Returns:
 *   Pointer to store, or NULL if queue doesn't exist
 */
hl7sql_store_t *hl7sql_queue_manager_get_queue(hl7sql_queue_manager_t *mgr,
                                                     const char *name);

/**
 * Check if a queue exists
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name to check
 *
 * Returns:
 *   1 if exists, 0 if not found or on error
 */
int hl7sql_queue_manager_has_queue(const hl7sql_queue_manager_t *mgr,
                                         const char *name);

/**
 * Check if a queue is empty
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name
 *
 * Returns:
 *   1 if empty, 0 if has messages or not found
 */
int hl7sql_queue_manager_is_empty(hl7sql_queue_manager_t *mgr,
                                      const char *name);

/**
 * Get list of all queue names
 *
 * Returns an array of queue names (dynamically allocated).
 * Caller must free both the array and individual strings.
 *
 * Args:
 *   mgr: Queue manager
 *   num: Output parameter for number of queues
 *
 * Returns:
 *   Array of queue names (NULL-terminated strings), or NULL on error
 *   Caller must free with free_queue_names()
 */
char **hl7sql_queue_manager_get_all_queues(hl7sql_queue_manager_t *mgr,
                                                 size_t *num);

/**
 * Free queue names array returned by get_all_queues
 *
 * Args:
 *   names: Array of queue names
 *   num: Number of names in array
 */
void hl7sql_queue_manager_free_queue_names(char **names, size_t num);

/* ====================================================================
 * History Operations
 * ==================================================================== */

/**
 * Get a queue's history message store
 *
 * History contains messages that have been popped/processed from the queue.
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name
 *
 * Returns:
 *   Pointer to history store, or NULL if queue doesn't exist
 */
hl7sql_store_t *hl7sql_queue_manager_get_history(hl7sql_queue_manager_t *mgr,
                                                     const char *name);

/**
 * Move a message to queue history
 *
 * Used internally by POP MESSAGE to preserve processed messages.
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name
 *   msg: Message to move to history (manager takes ownership)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int hl7sql_queue_manager_move_to_history(hl7sql_queue_manager_t *mgr,
                                             const char *name,
                                             hl7_message_t *msg);

/**
 * Clear all messages from queue history
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name
 *
 * Returns:
 *   0 on success, -1 if queue not found
 */
int hl7sql_queue_manager_clear_history(hl7sql_queue_manager_t *mgr,
                                           const char *name);

/**
 * Get history size (number of processed messages)
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name
 *
 * Returns:
 *   Number of messages in history, or 0 if not found
 */
size_t hl7sql_queue_manager_get_history_size(hl7sql_queue_manager_t *mgr,
                                                 const char *name);

/**
 * Rewrite queue persistence file after POP operation
 *
 * Atomically rewrites the persistence file to reflect the current
 * in-memory state of the queue after a message has been popped.
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int hl7sql_queue_manager_rewrite_persistence(hl7sql_queue_manager_t *mgr,
                                               const char *name);

#endif /* HL7SQL_QUEUE_MANAGER_H */
