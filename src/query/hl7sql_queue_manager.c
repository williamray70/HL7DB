/**
 * hl7sql_queue_manager.c - HL7SQL Queue Manager Implementation
 */

#define _GNU_SOURCE

#include "query/hl7sql_queue_manager.h"
#include "storage/persistence.h"
#include "util/logger.h"
#include <stdlib.h>
#include <string.h>

/* Forward declaration */
static int create_queue_internal(hl7sql_queue_manager_t *mgr,
                                  const char *name,
                                  bool register_in_persistence);

/* ====================================================================
 * Lifecycle Management
 * ==================================================================== */

hl7sql_queue_manager_t *hl7sql_queue_manager_create(void) {
    hl7sql_queue_manager_t *mgr = malloc(sizeof(hl7sql_queue_manager_t));
    if (!mgr) {
        LOG_ERROR("Failed to allocate queue manager");
        return NULL;
    }

    /* Create hashmap for queues */
    mgr->queues = hashmap_create(16);  /* Start with 16 buckets */
    if (!mgr->queues) {
        LOG_ERROR("Failed to create queues hashmap");
        free(mgr);
        return NULL;
    }

    /* Create hashmap for history queues */
    mgr->history_queues = hashmap_create(16);
    if (!mgr->history_queues) {
        LOG_ERROR("Failed to create history queues hashmap");
        hashmap_destroy(mgr->queues, NULL);
        free(mgr);
        return NULL;
    }

    /* Create hashmap for persistence handles */
    mgr->persistence_handles = hashmap_create(16);
    if (!mgr->persistence_handles) {
        LOG_ERROR("Failed to create persistence handles hashmap");
        hashmap_destroy(mgr->history_queues, NULL);
        hashmap_destroy(mgr->queues, NULL);
        free(mgr);
        return NULL;
    }

    /* Create hashmap for history persistence handles */
    mgr->history_persistence_handles = hashmap_create(16);
    if (!mgr->history_persistence_handles) {
        LOG_ERROR("Failed to create history persistence handles hashmap");
        hashmap_destroy(mgr->persistence_handles, NULL);
        hashmap_destroy(mgr->history_queues, NULL);
        hashmap_destroy(mgr->queues, NULL);
        free(mgr);
        return NULL;
    }

    /* Initialize RW lock */
    if (pthread_rwlock_init(&mgr->lock, NULL) != 0) {
        LOG_ERROR("Failed to initialize queue manager lock");
        hashmap_destroy(mgr->history_persistence_handles, NULL);
        hashmap_destroy(mgr->persistence_handles, NULL);
        hashmap_destroy(mgr->history_queues, NULL);
        hashmap_destroy(mgr->queues, NULL);
        free(mgr);
        return NULL;
    }

    /* No persistence by default */
    mgr->persistence = NULL;

    /* Default history configuration: unlimited */
    mgr->history_config.max_history_size = 0;  /* 0 = unlimited */
    mgr->history_config.auto_rotate = false;

    LOG_DEBUG("Created queue manager");
    return mgr;
}

void hl7sql_queue_manager_destroy(hl7sql_queue_manager_t *mgr) {
    if (!mgr) {
        return;
    }

    /* Destroy lock */
    pthread_rwlock_destroy(&mgr->lock);

    /* Close all persistence handles */
    if (mgr->persistence_handles) {
        hashmap_iter_t iter;
        if (hashmap_iter_init(mgr->persistence_handles, &iter)) {
            do {
                channel_persistence_t *cp = hashmap_iter_get_value(&iter);
                if (cp) {
                    persistence_close_channel(cp);
                }
            } while (hashmap_iter_next(&iter));
        }
        hashmap_destroy(mgr->persistence_handles, NULL);
    }

    /* Close all history persistence handles */
    if (mgr->history_persistence_handles) {
        hashmap_iter_t iter;
        if (hashmap_iter_init(mgr->history_persistence_handles, &iter)) {
            do {
                channel_persistence_t *cp = hashmap_iter_get_value(&iter);
                if (cp) {
                    persistence_close_channel(cp);
                }
            } while (hashmap_iter_next(&iter));
        }
        hashmap_destroy(mgr->history_persistence_handles, NULL);
    }

    /* Destroy hashmap and all stores/keys */
    if (mgr->queues) {
        /* Need to free both keys and values */
        hashmap_iter_t iter;
        if (hashmap_iter_init(mgr->queues, &iter)) {
            do {
                /* Get and free the key (queue name) */
                size_t key_len;
                const void *key = hashmap_iter_get_key(&iter, &key_len);
                if (key) {
                    free((void *)key);  /* Free strdup'd queue name */
                }

                /* Get and free the value (store) */
                hl7sql_store_t *store = hashmap_iter_get_value(&iter);
                if (store) {
                    hl7sql_store_destroy(store);
                }
            } while (hashmap_iter_next(&iter));
        }

        /* Destroy the hashmap itself (don't double-free) */
        hashmap_destroy(mgr->queues, NULL);
    }

    /* Destroy history hashmap and all history stores/keys */
    if (mgr->history_queues) {
        hashmap_iter_t iter;
        if (hashmap_iter_init(mgr->history_queues, &iter)) {
            do {
                /* Get and free the key (queue name) */
                size_t key_len;
                const void *key = hashmap_iter_get_key(&iter, &key_len);
                if (key) {
                    free((void *)key);  /* Free strdup'd queue name */
                }

                /* Get and free the value (history store) */
                hl7sql_store_t *store = hashmap_iter_get_value(&iter);
                if (store) {
                    hl7sql_store_destroy(store);
                }
            } while (hashmap_iter_next(&iter));
        }

        /* Destroy the hashmap itself (don't double-free) */
        hashmap_destroy(mgr->history_queues, NULL);
    }

    free(mgr);
    LOG_DEBUG("Destroyed queue manager");
}

int hl7sql_queue_manager_set_persistence(hl7sql_queue_manager_t *mgr,
                                             persistence_config_t *config) {
    if (!mgr || !config) {
        LOG_ERROR("Invalid arguments to set_persistence");
        return -1;
    }

    /* Initialize global persistence system */
    if (persistence_init(config) != 0) {
        LOG_ERROR("Failed to initialize persistence");
        return -1;
    }

    mgr->persistence = config;
    LOG_INFO("Persistence enabled: data_dir=%s", config->data_dir);
    return 0;
}

int hl7sql_queue_manager_set_history_config(hl7sql_queue_manager_t *mgr,
                                                size_t max_size,
                                                bool auto_rotate) {
    if (!mgr) {
        LOG_ERROR("Invalid arguments to set_history_config");
        return -1;
    }

    mgr->history_config.max_history_size = max_size;
    mgr->history_config.auto_rotate = auto_rotate;

    LOG_INFO("History config: max_size=%zu, auto_rotate=%s",
             max_size, auto_rotate ? "true" : "false");
    return 0;
}

int hl7sql_queue_manager_load_registry(hl7sql_queue_manager_t *mgr) {
    if (!mgr) {
        LOG_ERROR("Invalid arguments to load_registry");
        return -1;
    }

    /* Only load if persistence is enabled */
    if (!mgr->persistence) {
        LOG_DEBUG("Persistence not enabled, skipping registry load");
        return 0;
    }

    /* Load channel names from registry */
    size_t count = 0;
    char **names = persistence_load_channel_registry(&count);

    if (!names || count == 0) {
        /* No channels to load */
        if (names) {
            persistence_free_channel_registry(names, count);
        }
        return 0;
    }

    LOG_INFO("Loading %zu queues from registry", count);

    /* Create each queue */
    int loaded = 0;
    for (size_t i = 0; i < count; i++) {
        const char *queue_name = names[i];

        /* Check if queue already exists (shouldn't happen, but be safe) */
        if (hl7sql_queue_manager_has_queue(mgr, queue_name)) {
            LOG_WARN("Queue '%s' already exists, skipping", queue_name);
            continue;
        }

        /* Create the queue without re-registering (this will load persisted messages) */
        int result = create_queue_internal(mgr, queue_name, false);
        if (result == 0) {
            loaded++;
        } else if (result == -1) {
            /* Queue already exists - shouldn't happen */
            LOG_WARN("Queue '%s' already exists during registry load", queue_name);
        } else {
            LOG_ERROR("Failed to create queue '%s' from registry", queue_name);
        }
    }

    persistence_free_channel_registry(names, count);

    LOG_INFO("Successfully loaded %d/%zu queues from registry", loaded, count);
    return loaded;
}

/* ====================================================================
 * Queue Operations
 * ==================================================================== */

/**
 * Internal helper for creating queues
 *
 * Args:
 *   mgr: Queue manager
 *   name: Queue name
 *   register_in_persistence: Whether to register in channels.dat registry
 */
static int create_queue_internal(hl7sql_queue_manager_t *mgr,
                                  const char *name,
                                  bool register_in_persistence) {
    if (!mgr || !name) {
        LOG_ERROR("Invalid arguments to create_queue");
        return -2;
    }

    /* Acquire write lock */
    pthread_rwlock_wrlock(&mgr->lock);

    /* Check if already exists */
    if (hashmap_contains(mgr->queues, name)) {
        pthread_rwlock_unlock(&mgr->lock);
        LOG_WARN("Queue '%s' already exists", name);
        return -1;
    }

    /* Create new store */
    hl7sql_store_t *store = hl7sql_store_create();
    if (!store) {
        pthread_rwlock_unlock(&mgr->lock);
        LOG_ERROR("Failed to create store for queue '%s'", name);
        return -2;
    }

    /* Create history store */
    hl7sql_store_t *history_store = hl7sql_store_create();
    if (!history_store) {
        hl7sql_store_destroy(store);
        pthread_rwlock_unlock(&mgr->lock);
        LOG_ERROR("Failed to create history store for queue '%s'", name);
        return -2;
    }

    /* Open persistence if enabled */
    channel_persistence_t *persistence_handle = NULL;
    channel_persistence_t *history_persistence_handle = NULL;
    if (mgr->persistence) {
        /* Main queue persistence */
        persistence_handle = persistence_open_channel(name);
        if (!persistence_handle) {
            LOG_WARN("Failed to open persistence for queue '%s'", name);
            /* Continue without persistence for this queue */
        } else {
            /* Load existing messages from disk */
            list_t *persisted_msgs = persistence_load_messages(persistence_handle);
            if (persisted_msgs) {
                /* Insert each message into the store */
                list_node_t *node = persisted_msgs->head;
                size_t loaded_count = 0;
                while (node) {
                    hl7_message_t *msg = (hl7_message_t *)node->data;
                    hl7sql_store_insert(store, msg);
                    loaded_count++;
                    node = node->next;
                }
                LOG_INFO("Loaded %zu persisted messages for queue '%s'", loaded_count, name);
                /* Free the list (but NOT the messages - store owns them now) */
                list_destroy(persisted_msgs, NULL);
            }

            /* Store persistence handle */
            hashmap_put(mgr->persistence_handles, name, persistence_handle);
        }

        /* History persistence */
        char history_name[256];
        snprintf(history_name, sizeof(history_name), "%s.history", name);
        history_persistence_handle = persistence_open_channel(history_name);
        if (!history_persistence_handle) {
            LOG_WARN("Failed to open history persistence for queue '%s'", name);
        } else {
            /* Load existing history messages from disk */
            list_t *persisted_history = persistence_load_messages(history_persistence_handle);
            if (persisted_history) {
                list_node_t *node = persisted_history->head;
                size_t loaded_count = 0;
                while (node) {
                    hl7_message_t *msg = (hl7_message_t *)node->data;
                    hl7sql_store_insert(history_store, msg);
                    loaded_count++;
                    node = node->next;
                }
                LOG_INFO("Loaded %zu persisted history messages for queue '%s'", loaded_count, name);
                list_destroy(persisted_history, NULL);
            }

            /* Store history persistence handle */
            hashmap_put(mgr->history_persistence_handles, name, history_persistence_handle);
        }
    }

    /* Duplicate queue name (hashmap doesn't copy keys) */
    char *name_copy = strdup(name);
    if (!name_copy) {
        if (history_persistence_handle) {
            persistence_close_channel(history_persistence_handle);
        }
        if (persistence_handle) {
            persistence_close_channel(persistence_handle);
        }
        hl7sql_store_destroy(history_store);
        hl7sql_store_destroy(store);
        pthread_rwlock_unlock(&mgr->lock);
        LOG_ERROR("Failed to duplicate queue name '%s'", name);
        return -2;
    }

    /* Duplicate for history map */
    char *history_name_copy = strdup(name);
    if (!history_name_copy) {
        free(name_copy);
        if (history_persistence_handle) {
            persistence_close_channel(history_persistence_handle);
        }
        if (persistence_handle) {
            persistence_close_channel(persistence_handle);
        }
        hl7sql_store_destroy(history_store);
        hl7sql_store_destroy(store);
        pthread_rwlock_unlock(&mgr->lock);
        LOG_ERROR("Failed to duplicate history queue name '%s'", name);
        return -2;
    }

    /* Add to hashmaps */
    hashmap_put(mgr->queues, name_copy, store);
    hashmap_put(mgr->history_queues, history_name_copy, history_store);

    /* Register in persistence registry if enabled and requested */
    if (register_in_persistence && mgr->persistence) {
        if (persistence_register_channel(name) != 0) {
            LOG_WARN("Failed to register queue '%s' in registry", name);
            /* Don't fail the operation - queue is already created in memory */
        }
    }

    pthread_rwlock_unlock(&mgr->lock);
    LOG_INFO("Created queue '%s' with history support", name);
    return 0;
}

int hl7sql_queue_manager_create_queue(hl7sql_queue_manager_t *mgr,
                                            const char *name) {
    return create_queue_internal(mgr, name, true);
}

int hl7sql_queue_manager_drop_queue(hl7sql_queue_manager_t *mgr,
                                          const char *name) {
    if (!mgr || !name) {
        LOG_ERROR("Invalid arguments to drop_queue");
        return -1;
    }

    /* Acquire write lock */
    pthread_rwlock_wrlock(&mgr->lock);

    /* Check if exists */
    hl7sql_store_t *store = hashmap_get(mgr->queues, name);
    if (!store) {
        pthread_rwlock_unlock(&mgr->lock);
        LOG_WARN("Queue '%s' not found", name);
        return -1;
    }

    /* Check if empty (safety requirement) */
    hl7sql_store_read_lock(store);
    list_t *messages = hl7sql_store_get_all(store);
    size_t count = messages ? messages->size : 0;
    hl7sql_store_read_unlock(store);

    if (count > 0) {
        pthread_rwlock_unlock(&mgr->lock);
        LOG_WARN("Cannot drop non-empty queue '%s' (%zu messages)", name, count);
        return -2;
    }

    /* Remove from hashmap (returns the old key) */
    /* We need to free both the key and value */
    hashmap_iter_t iter;
    char *found_key = NULL;

    /* Find the actual key pointer used in hashmap */
    if (hashmap_iter_init(mgr->queues, &iter)) {
        do {
            size_t key_len;
            const char *key = hashmap_iter_get_key(&iter, &key_len);
            if (key && strcmp(key, name) == 0) {
                found_key = (char *)key;
                break;
            }
        } while (hashmap_iter_next(&iter));
    }

    /* Remove from map */
    hl7sql_store_t *removed_store = hashmap_remove(mgr->queues, name);

    /* Free the store */
    if (removed_store) {
        hl7sql_store_destroy(removed_store);
    }

    /* Free the key */
    if (found_key) {
        free(found_key);
    }

    /* Unregister from persistence registry if enabled */
    if (mgr->persistence) {
        if (persistence_unregister_channel(name) != 0) {
            LOG_WARN("Failed to unregister queue '%s' from registry", name);
            /* Don't fail the operation - queue is already removed from memory */
        }
    }

    pthread_rwlock_unlock(&mgr->lock);
    LOG_INFO("Dropped queue '%s'", name);
    return 0;
}

hl7sql_store_t *hl7sql_queue_manager_get_queue(hl7sql_queue_manager_t *mgr,
                                                     const char *name) {
    if (!mgr || !name) {
        return NULL;
    }

    /* Acquire read lock */
    pthread_rwlock_rdlock(&mgr->lock);
    hl7sql_store_t *store = hashmap_get(mgr->queues, name);
    pthread_rwlock_unlock(&mgr->lock);

    return store;
}

int hl7sql_queue_manager_has_queue(const hl7sql_queue_manager_t *mgr,
                                         const char *name) {
    if (!mgr || !name) {
        return 0;
    }

    /* Acquire read lock */
    pthread_rwlock_rdlock((pthread_rwlock_t *)&mgr->lock);
    int exists = hashmap_contains(mgr->queues, name);
    pthread_rwlock_unlock((pthread_rwlock_t *)&mgr->lock);

    return exists;
}

int hl7sql_queue_manager_is_empty(hl7sql_queue_manager_t *mgr,
                                      const char *name) {
    if (!mgr || !name) {
        return 0;
    }

    /* Get the store */
    hl7sql_store_t *store = hl7sql_queue_manager_get_queue(mgr, name);
    if (!store) {
        return 0;  /* Not found = not empty (for safety) */
    }

    /* Check if store is empty */
    hl7sql_store_read_lock(store);
    list_t *messages = hl7sql_store_get_all(store);
    int is_empty = (messages == NULL || messages->size == 0);
    hl7sql_store_read_unlock(store);

    return is_empty;
}

char **hl7sql_queue_manager_get_all_queues(hl7sql_queue_manager_t *mgr,
                                                 size_t *num) {
    if (!mgr || !num) {
        if (num) *num = 0;
        return NULL;
    }

    pthread_rwlock_rdlock(&mgr->lock);

    size_t count = hashmap_size(mgr->queues);
    *num = count;

    if (count == 0) {
        pthread_rwlock_unlock(&mgr->lock);
        return NULL;
    }

    /* Allocate array for queue names */
    char **names = malloc(count * sizeof(char *));
    if (!names) {
        pthread_rwlock_unlock(&mgr->lock);
        *num = 0;
        LOG_ERROR("Failed to allocate queue names array");
        return NULL;
    }

    /* Iterate and collect queue names */
    size_t index = 0;
    hashmap_iter_t iter;
    if (hashmap_iter_init(mgr->queues, &iter)) {
        do {
            size_t key_len;
            const char *key = hashmap_iter_get_key(&iter, &key_len);
            if (key && index < count) {
                names[index] = strdup(key);
                if (!names[index]) {
                    /* Allocation failed - clean up */
                    for (size_t i = 0; i < index; i++) {
                        free(names[i]);
                    }
                    free(names);
                    pthread_rwlock_unlock(&mgr->lock);
                    *num = 0;
                    LOG_ERROR("Failed to duplicate queue name");
                    return NULL;
                }
                index++;
            }
        } while (hashmap_iter_next(&iter));
    }

    pthread_rwlock_unlock(&mgr->lock);
    LOG_DEBUG("Retrieved %zu queue names", count);
    return names;
}

void hl7sql_queue_manager_free_queue_names(char **names, size_t num) {
    if (!names) {
        return;
    }

    for (size_t i = 0; i < num; i++) {
        free(names[i]);
    }
    free(names);
}

/* ====================================================================
 * History Operations
 * ==================================================================== */

hl7sql_store_t *hl7sql_queue_manager_get_history(hl7sql_queue_manager_t *mgr,
                                                     const char *name) {
    if (!mgr || !name) {
        return NULL;
    }

    /* Acquire read lock */
    pthread_rwlock_rdlock(&mgr->lock);
    hl7sql_store_t *history_store = hashmap_get(mgr->history_queues, name);
    pthread_rwlock_unlock(&mgr->lock);

    return history_store;
}

int hl7sql_queue_manager_move_to_history(hl7sql_queue_manager_t *mgr,
                                             const char *name,
                                             hl7_message_t *msg) {
    if (!mgr || !name || !msg) {
        LOG_ERROR("Invalid arguments to move_to_history");
        return -1;
    }

    /* Get history store */
    hl7sql_store_t *history_store = hl7sql_queue_manager_get_history(mgr, name);
    if (!history_store) {
        LOG_ERROR("History store not found for queue '%s'", name);
        return -1;
    }

    /* Check history size limit */
    size_t max_size = mgr->history_config.max_history_size;
    if (max_size > 0) {
        size_t current_size = hl7sql_store_size(history_store);

        if (current_size >= max_size) {
            if (mgr->history_config.auto_rotate) {
                /* Auto-rotate: remove oldest message */
                hl7sql_store_write_lock(history_store);
                list_t *messages = hl7sql_store_get_all(history_store);
                if (messages && messages->size > 0) {
                    hl7_message_t *oldest = list_remove_first(messages);
                    if (oldest) {
                        LOG_DEBUG("Auto-rotated oldest message (ID %lu) from history '%s'",
                                  (unsigned long)oldest->msg_id, name);
                        hl7_message_destroy(oldest);
                    }
                }
                hl7sql_store_write_unlock(history_store);
            } else {
                /* No auto-rotate: reject new message (caller must destroy) */
                LOG_WARN("History for queue '%s' is full (%zu/%zu), message rejected",
                         name, current_size, max_size);
                return -2;  /* Special code: history full, caller keeps ownership of msg */
            }
        }
    }

    /* Insert message into history */
    if (hl7sql_store_insert(history_store, msg) != 0) {
        LOG_ERROR("Failed to insert message into history for queue '%s'", name);
        return -1;
    }

    /* Persist to disk if enabled */
    if (mgr->persistence) {
        channel_persistence_t *history_handle = hashmap_get(mgr->history_persistence_handles, name);
        if (history_handle) {
            if (persistence_append_message(history_handle, msg->msg_id, msg) != 0) {
                LOG_WARN("Failed to persist message to history for queue '%s'", name);
                /* Continue - message is in memory at least */
            }
        }
    }

    LOG_DEBUG("Moved message ID %lu to history for queue '%s'",
              (unsigned long)msg->msg_id, name);
    return 0;
}

int hl7sql_queue_manager_clear_history(hl7sql_queue_manager_t *mgr,
                                           const char *name) {
    if (!mgr || !name) {
        LOG_ERROR("Invalid arguments to clear_history");
        return -1;
    }

    /* Get history store */
    hl7sql_store_t *history_store = hl7sql_queue_manager_get_history(mgr, name);
    if (!history_store) {
        LOG_WARN("History store not found for queue '%s'", name);
        return -1;
    }

    /* Clear the history store */
    hl7sql_store_clear(history_store);

    /* Clear persisted history if enabled */
    if (mgr->persistence) {
        channel_persistence_t *history_handle = hashmap_get(mgr->history_persistence_handles, name);
        if (history_handle) {
            /* Close and reopen to clear the file */
            char history_name[256];
            snprintf(history_name, sizeof(history_name), "%s.history", name);
            persistence_close_channel(history_handle);

            /* Reopen (will create fresh file) */
            channel_persistence_t *new_handle = persistence_open_channel(history_name);
            if (new_handle) {
                hashmap_put(mgr->history_persistence_handles, name, new_handle);
            }
        }
    }

    LOG_INFO("Cleared history for queue '%s'", name);
    return 0;
}

size_t hl7sql_queue_manager_get_history_size(hl7sql_queue_manager_t *mgr,
                                                 const char *name) {
    if (!mgr || !name) {
        return 0;
    }

    hl7sql_store_t *history_store = hl7sql_queue_manager_get_history(mgr, name);
    if (!history_store) {
        return 0;
    }

    return hl7sql_store_size(history_store);
}

int hl7sql_queue_manager_rewrite_persistence(hl7sql_queue_manager_t *mgr,
                                               const char *name) {
    if (!mgr || !name) {
        LOG_ERROR("Invalid arguments to rewrite_persistence");
        return -1;
    }

    /* Only rewrite if persistence is enabled */
    if (!mgr->persistence) {
        return 0;  /* Not an error, just no-op */
    }

    /* Get persistence handle for this queue */
    channel_persistence_t *persistence_handle = hashmap_get(mgr->persistence_handles, name);
    if (!persistence_handle) {
        LOG_WARN("No persistence handle found for queue '%s' during rewrite", name);
        return 0;  /* Not an error - queue might not have persistence */
    }

    /* Get the current queue store */
    hl7sql_store_t *store = hl7sql_queue_manager_get_queue(mgr, name);
    if (!store) {
        LOG_ERROR("Queue '%s' not found during persistence rewrite", name);
        return -1;
    }

    /* Get current message list (read lock to avoid race) */
    hl7sql_store_read_lock(store);
    list_t *messages = hl7sql_store_get_all(store);

    /* Rewrite the persistence file with current messages */
    int result = persistence_rewrite_messages(persistence_handle, messages);

    hl7sql_store_read_unlock(store);

    if (result != 0) {
        LOG_ERROR("Failed to rewrite persistence for queue '%s'", name);
        return -1;
    }

    LOG_DEBUG("Successfully rewrote persistence for queue '%s'", name);
    return 0;
}
