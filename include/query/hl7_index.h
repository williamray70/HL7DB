/**
 * hl7_index.h - HL7 Channel Index Structures
 *
 * Maintains pre-built indexes on the six most common HL7 query fields.
 * Every INSERT updates all indexes; every POP removes from all indexes.
 * Queries that target an indexed field never perform a table scan.
 *
 * Indexed fields:
 *   PID[3]   - Patient ID (MRN)       - hash  - equality
 *   MSH[9]   - Message type           - hash  - equality
 *   MSH[3]   - Sending application    - hash  - equality
 *   MSH[10]  - Message control ID     - hash  - equality (unique)
 *   PV1[19]  - Visit/encounter number - hash  - equality
 *   MSH[7]   - Message timestamp      - sorted array - range queries
 *
 * Non-indexed fields still work via full scan with a logged warning.
 */

#ifndef HL7DB_HL7_INDEX_H
#define HL7DB_HL7_INDEX_H

#include "hl7/hl7_message.h"
#include "util/hashmap.h"
#include "util/list.h"
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

/* -----------------------------------------------------------------------
 * Timestamp index entry (one per message with a valid MSH-7)
 * ----------------------------------------------------------------------- */
typedef struct {
    time_t  ts;         /* Parsed MSH-7 value as Unix time          */
    uint64_t msg_id;    /* Matches hl7_message_t.msg_id             */
} ts_entry_t;

/* -----------------------------------------------------------------------
 * Sorted timestamp index
 *
 * Kept sorted by ts ascending using insertion sort on insert (messages
 * arrive roughly in order so insertion sort is O(1) amortized).
 * Range queries use binary search: O(log n) to find bounds, O(k) to
 * collect results.
 * ----------------------------------------------------------------------- */
typedef struct {
    ts_entry_t *entries;    /* Sorted array of (ts, msg_id) pairs   */
    size_t      count;      /* Number of valid entries              */
    size_t      capacity;   /* Allocated capacity                   */
} hl7_ts_index_t;

/* -----------------------------------------------------------------------
 * Channel index
 *
 * Owns no hl7_message_t pointers; those live in the master map.
 * Hash indexes map field-value -> list_t* of uint64_t* (msg_id copies).
 * The msg_id copies are heap-allocated and freed when removed.
 * ----------------------------------------------------------------------- */
typedef struct {
    /* Hash indexes: field value (char*) -> list_t* of uint64_t* msg_ids */
    hashmap_t      *by_pid3;    /* PID[3]  - patient ID              */
    hashmap_t      *by_msh9;    /* MSH[9]  - message type            */
    hashmap_t      *by_msh3;    /* MSH[3]  - sending application     */
    hashmap_t      *by_msh10;   /* MSH[10] - message control ID (unique) */
    hashmap_t      *by_pv119;   /* PV1[19] - visit/encounter number  */

    /* Timestamp range index */
    hl7_ts_index_t *by_msh7;   /* MSH[7]  - message timestamp       */
} hl7_channel_index_t;

/* -----------------------------------------------------------------------
 * Lifecycle
 * ----------------------------------------------------------------------- */

/**
 * Allocate and initialise a channel index.
 * Returns NULL on allocation failure.
 */
hl7_channel_index_t *hl7_index_create(void);

/**
 * Destroy index and free all memory.
 * Does NOT free the hl7_message_t objects themselves.
 */
void hl7_index_destroy(hl7_channel_index_t *idx);

/* -----------------------------------------------------------------------
 * Mutation
 * ----------------------------------------------------------------------- */

/**
 * Add a message to all applicable indexes.
 * Called from hl7sql_store_insert() after msg_id is assigned.
 * Fields that are absent or empty are silently skipped.
 *
 * @param idx   Channel index
 * @param msg   Fully parsed message with msg_id assigned
 * @return 0 on success, -1 on allocation failure (partial update may occur)
 */
int hl7_index_add(hl7_channel_index_t *idx, const hl7_message_t *msg);

/**
 * Remove a message from all indexes.
 * Called from hl7sql_store_pop() before the message is destroyed.
 *
 * @param idx    Channel index
 * @param msg    Message to remove (uses msg_id + field values to locate entries)
 */
void hl7_index_remove(hl7_channel_index_t *idx, const hl7_message_t *msg);

/* -----------------------------------------------------------------------
 * Lookup
 * ----------------------------------------------------------------------- */

/**
 * Describe which field a WHERE clause targets, used by the query planner.
 */
typedef enum {
    HL7_IDX_FIELD_NONE   = 0,  /* Not an indexed field (fallback to scan) */
    HL7_IDX_FIELD_PID3   = 1,
    HL7_IDX_FIELD_MSH9   = 2,
    HL7_IDX_FIELD_MSH3   = 3,
    HL7_IDX_FIELD_MSH10  = 4,
    HL7_IDX_FIELD_PV119  = 5,
    HL7_IDX_FIELD_MSH7   = 6,  /* Timestamp - range queries only */
} hl7_index_field_t;

/**
 * Classify a segment/field pair as an indexed field.
 *
 * @param segment_id  Segment identifier, e.g. "PID"
 * @param field_num   1-based field number matching the grammar's [N] syntax
 * @return            Corresponding hl7_index_field_t, or HL7_IDX_FIELD_NONE
 */
hl7_index_field_t hl7_index_classify(const char *segment_id, int field_num);

/**
 * Equality lookup on a hash index.
 *
 * Returns a list of uint64_t* msg_ids matching the value, or NULL if none.
 * The returned list is owned by the index - do NOT free it.
 * Caller must hold the store's read lock while using the result.
 *
 * @param idx    Channel index
 * @param field  Which indexed field to query (must not be HL7_IDX_FIELD_MSH7)
 * @param value  Value to match (case-sensitive)
 * @return       list_t* of uint64_t* msg_ids, or NULL
 */
const list_t *hl7_index_lookup_eq(const hl7_channel_index_t *idx,
                                   hl7_index_field_t field,
                                   const char *value);

/**
 * Range lookup on the timestamp index.
 *
 * Returns a freshly allocated list_t* of uint64_t* msg_id copies for
 * messages where lower_ts <= MSH-7 <= upper_ts.
 * Caller must free the list (but NOT the msg_id pointers inside - those
 * are owned by the index).
 *
 * Actually: to keep ownership simple the returned list contains copies;
 * caller must free both the list and each uint64_t* element.
 *
 * @param idx       Channel index
 * @param lower_ts  Lower bound (inclusive), Unix time
 * @param upper_ts  Upper bound (inclusive), Unix time
 * @return          Newly allocated list_t* of uint64_t* (caller frees), or NULL
 */
list_t *hl7_index_lookup_ts_range(const hl7_channel_index_t *idx,
                                   time_t lower_ts,
                                   time_t upper_ts);

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */

/**
 * Parse an HL7 timestamp string (DTM format: YYYYMMDDHHMMSS[+-ZZZZ])
 * into a Unix time_t.
 *
 * @param dtm   HL7 DTM string, e.g. "20230615083000"
 * @return      Unix time, or 0 on parse failure
 */
time_t hl7_parse_dtm(const char *dtm);

/**
 * Return a human-readable name for a field (for error messages).
 */
const char *hl7_index_field_name(hl7_index_field_t field);

#endif /* HL7DB_HL7_INDEX_H */
