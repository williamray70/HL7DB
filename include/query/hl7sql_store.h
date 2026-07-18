/**
 * hl7sql_store.h - In-Memory Message Store
 *
 * Stores HL7 messages in a master hashmap (msg_id -> hl7_message_t*) and
 * maintains six pre-built indexes for O(1)/O(log n) queries without scans.
 */

#ifndef HL7DB_HL7SQL_STORE_H
#define HL7DB_HL7SQL_STORE_H

#include "hl7/hl7_types.h"
#include "hl7/hl7_message.h"
#include "util/hashmap.h"
#include "util/list.h"
#include "query/hl7_index.h"
#include <stdint.h>
#include <pthread.h>

/**
 * In-Memory Message Store
 *
 * master     - owns all hl7_message_t* pointers (msg_id -> hl7_message_t*)
 * messages   - insertion-ordered list of hl7_message_t* for FIFO POP and
 *              SELECT * (no WHERE) iteration; does NOT own the pointers
 * index      - six pre-built field indexes; updated on every insert/remove
 */
typedef struct {
    hashmap_t          *master;     /* msg_id (uint64_t) -> hl7_message_t*  */
    list_t             *messages;   /* Insertion order, for POP / full scan  */
    hl7_channel_index_t *index;     /* Pre-built field indexes               */
    pthread_rwlock_t    lock;
    uint64_t            next_id;
} hl7sql_store_t;

/* -----------------------------------------------------------------------
 * Lifecycle
 * ----------------------------------------------------------------------- */
hl7sql_store_t *hl7sql_store_create(void);
void            hl7sql_store_destroy(hl7sql_store_t *store);
void            hl7sql_store_clear(hl7sql_store_t *store);

/* -----------------------------------------------------------------------
 * Mutation (caller must NOT hold the store lock)
 * ----------------------------------------------------------------------- */

/**
 * Insert a message.  Store assigns msg_id and updates all indexes.
 * Store takes ownership of msg on success.
 */
int  hl7sql_store_insert(hl7sql_store_t *store, hl7_message_t *msg);

/**
 * Pop the oldest message (FIFO).
 * Removes from master map, ordered list, and all indexes.
 * Returns the message pointer (caller owns it and must hl7_message_destroy it),
 * or NULL if the store is empty.
 */
hl7_message_t *hl7sql_store_pop(hl7sql_store_t *store);

/* -----------------------------------------------------------------------
 * Read access (caller must hold read lock)
 * ----------------------------------------------------------------------- */

/**
 * Look up a single message by msg_id.
 * Returns borrowed pointer; do NOT free.  Caller must hold read lock.
 */
hl7_message_t *hl7sql_store_get_by_id(hl7sql_store_t *store, uint64_t msg_id);

/**
 * Return insertion-ordered list (for full-channel iteration).
 * Caller must hold read lock; do NOT free the list.
 */
list_t *hl7sql_store_get_all(hl7sql_store_t *store);

/**
 * Return the channel index for query planning.
 * Caller must hold read lock.
 */
const hl7_channel_index_t *hl7sql_store_get_index(const hl7sql_store_t *store);

size_t hl7sql_store_size(const hl7sql_store_t *store);

/* -----------------------------------------------------------------------
 * Locking
 * ----------------------------------------------------------------------- */
int hl7sql_store_read_lock(hl7sql_store_t *store);
int hl7sql_store_read_unlock(hl7sql_store_t *store);
int hl7sql_store_write_lock(hl7sql_store_t *store);
int hl7sql_store_write_unlock(hl7sql_store_t *store);

#endif /* HL7DB_HL7SQL_STORE_H */
