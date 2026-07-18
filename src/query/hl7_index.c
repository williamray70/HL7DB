/**
 * hl7_index.c - HL7 Channel Index Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include "query/hl7_index.h"
#include "util/logger.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* -----------------------------------------------------------------------
 * Timestamp index internals
 * ----------------------------------------------------------------------- */

#define TS_INDEX_INITIAL_CAP 64

static hl7_ts_index_t *ts_index_create(void) {
    hl7_ts_index_t *t = calloc(1, sizeof(hl7_ts_index_t));
    if (!t) return NULL;

    t->entries  = malloc(TS_INDEX_INITIAL_CAP * sizeof(ts_entry_t));
    if (!t->entries) { free(t); return NULL; }
    t->count    = 0;
    t->capacity = TS_INDEX_INITIAL_CAP;
    return t;
}

static void ts_index_destroy(hl7_ts_index_t *t) {
    if (!t) return;
    free(t->entries);
    free(t);
}

/* Insert keeping entries sorted by ts ascending.
 * Messages usually arrive in order so this is typically O(1). */
static int ts_index_insert(hl7_ts_index_t *t, time_t ts, uint64_t msg_id) {
    /* Grow if needed */
    if (t->count == t->capacity) {
        size_t new_cap = t->capacity * 2;
        ts_entry_t *grown = realloc(t->entries, new_cap * sizeof(ts_entry_t));
        if (!grown) return -1;
        t->entries  = grown;
        t->capacity = new_cap;
    }

    /* Find insertion point (binary search) */
    size_t lo = 0, hi = t->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (t->entries[mid].ts <= ts) lo = mid + 1;
        else                          hi = mid;
    }

    /* Shift right and insert */
    if (lo < t->count) {
        memmove(&t->entries[lo + 1], &t->entries[lo],
                (t->count - lo) * sizeof(ts_entry_t));
    }
    t->entries[lo].ts     = ts;
    t->entries[lo].msg_id = msg_id;
    t->count++;
    return 0;
}

/* Remove by msg_id (linear scan - removals are infrequent POP operations) */
static void ts_index_remove(hl7_ts_index_t *t, uint64_t msg_id) {
    for (size_t i = 0; i < t->count; i++) {
        if (t->entries[i].msg_id == msg_id) {
            if (i + 1 < t->count) {
                memmove(&t->entries[i], &t->entries[i + 1],
                        (t->count - i - 1) * sizeof(ts_entry_t));
            }
            t->count--;
            return;
        }
    }
}

/* -----------------------------------------------------------------------
 * Hash index helpers
 *
 * Each hash index maps:  char* field_value -> list_t* of uint64_t* msg_ids
 *
 * The char* keys are heap-allocated when first seen and freed on removal
 * when their list becomes empty.  The uint64_t* values in each list are
 * individually heap-allocated so list_remove() can compare by pointer.
 * ----------------------------------------------------------------------- */

/* Free a list of uint64_t* msg_id entries (used when destroying a bucket) */
static void free_id_list(void *ptr) {
    list_t *lst = (list_t *)ptr;
    if (!lst) return;
    list_node_t *node = lst->head;
    while (node) {
        free(node->data);   /* free the uint64_t* */
        node = node->next;
    }
    list_destroy(lst, NULL);
}

/* Add msg_id to the list stored at key in map.
 * Creates the list and inserts the key if not present.
 * key must be a heap-allocated string owned by the caller (we strdup). */
static int hash_index_add(hashmap_t *map, const char *key, uint64_t msg_id) {
    if (!key || !*key) return 0; /* skip empty values */

    list_t *lst = hashmap_get(map, key);
    if (!lst) {
        lst = list_create();
        if (!lst) return -1;
        char *key_copy = strdup(key);
        if (!key_copy) { list_destroy(lst, NULL); return -1; }
        hashmap_put(map, key_copy, lst);
    }

    uint64_t *id_copy = malloc(sizeof(uint64_t));
    if (!id_copy) return -1;
    *id_copy = msg_id;

    if (list_append(lst, id_copy) != 0) { free(id_copy); return -1; }
    return 0;
}

/* Remove msg_id from the list at key.  If the list becomes empty, remove
 * the key from the map and free both the list and the key string. */
static void hash_index_remove(hashmap_t *map, const char *key, uint64_t msg_id) {
    if (!key || !*key) return;

    list_t *lst = hashmap_get(map, key);
    if (!lst) return;

    /* Find and remove the matching uint64_t node */
    list_node_t *node = lst->head;
    while (node) {
        uint64_t *id = (uint64_t *)node->data;
        if (*id == msg_id) {
            list_remove(lst, node->data);
            free(id);
            break;
        }
        node = node->next;
    }

    /* If list is now empty, remove key from map and free both */
    if (lst->size == 0) {
        /* We need the actual key pointer stored in the map to free it */
        hashmap_iter_t iter;
        if (hashmap_iter_init(map, &iter)) {
            do {
                size_t klen;
                const char *k = hashmap_iter_get_key(&iter, &klen);
                if (k && strcmp(k, key) == 0) {
                    hashmap_remove(map, key);
                    free((void *)k);
                    break;
                }
            } while (hashmap_iter_next(&iter));
        }
        list_destroy(lst, NULL);
    }
}

/* -----------------------------------------------------------------------
 * Lifecycle
 * ----------------------------------------------------------------------- */

hl7_channel_index_t *hl7_index_create(void) {
    hl7_channel_index_t *idx = calloc(1, sizeof(hl7_channel_index_t));
    if (!idx) return NULL;

    idx->by_pid3  = hashmap_create(64);
    idx->by_msh9  = hashmap_create(16);
    idx->by_msh3  = hashmap_create(16);
    idx->by_msh10 = hashmap_create(64);
    idx->by_pv119 = hashmap_create(64);
    idx->by_msh7  = ts_index_create();

    if (!idx->by_pid3 || !idx->by_msh9 || !idx->by_msh3 ||
        !idx->by_msh10 || !idx->by_pv119 || !idx->by_msh7) {
        hl7_index_destroy(idx);
        return NULL;
    }

    LOG_DEBUG("Created channel index");
    return idx;
}

void hl7_index_destroy(hl7_channel_index_t *idx) {
    if (!idx) return;

    if (idx->by_pid3)  hashmap_destroy(idx->by_pid3,  free_id_list);
    if (idx->by_msh9)  hashmap_destroy(idx->by_msh9,  free_id_list);
    if (idx->by_msh3)  hashmap_destroy(idx->by_msh3,  free_id_list);
    if (idx->by_msh10) hashmap_destroy(idx->by_msh10, free_id_list);
    if (idx->by_pv119) hashmap_destroy(idx->by_pv119, free_id_list);
    if (idx->by_msh7)  ts_index_destroy(idx->by_msh7);

    free(idx);
    LOG_DEBUG("Destroyed channel index");
}

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */

hl7_index_field_t hl7_index_classify(const char *segment_id, int field_num) {
    if (!segment_id) return HL7_IDX_FIELD_NONE;

    if (strcmp(segment_id, "PID") == 0 && field_num == 3)  return HL7_IDX_FIELD_PID3;
    if (strcmp(segment_id, "MSH") == 0 && field_num == 9)  return HL7_IDX_FIELD_MSH9;
    if (strcmp(segment_id, "MSH") == 0 && field_num == 3)  return HL7_IDX_FIELD_MSH3;
    if (strcmp(segment_id, "MSH") == 0 && field_num == 10) return HL7_IDX_FIELD_MSH10;
    if (strcmp(segment_id, "MSH") == 0 && field_num == 7)  return HL7_IDX_FIELD_MSH7;
    if (strcmp(segment_id, "PV1") == 0 && field_num == 19) return HL7_IDX_FIELD_PV119;

    return HL7_IDX_FIELD_NONE;
}

const char *hl7_index_field_name(hl7_index_field_t field) {
    switch (field) {
        case HL7_IDX_FIELD_PID3:  return "PID[3] (Patient ID)";
        case HL7_IDX_FIELD_MSH9:  return "MSH[9] (Message Type)";
        case HL7_IDX_FIELD_MSH3:  return "MSH[3] (Sending Application)";
        case HL7_IDX_FIELD_MSH10: return "MSH[10] (Message Control ID)";
        case HL7_IDX_FIELD_PV119: return "PV1[19] (Visit Number)";
        case HL7_IDX_FIELD_MSH7:  return "MSH[7] (Timestamp)";
        default:                   return "unindexed field";
    }
}

/* Parse HL7 DTM: YYYY[MM[DD[HH[MM[SS]]]]][+/-ZZZZ]
 * Returns 0 on failure. */
time_t hl7_parse_dtm(const char *dtm) {
    if (!dtm || strlen(dtm) < 8) return 0;

    struct tm t = {0};
    /* YYYYMMDD mandatory */
    char buf[5];
    memcpy(buf, dtm,     4); buf[4] = '\0'; t.tm_year = atoi(buf) - 1900;
    memcpy(buf, dtm + 4, 2); buf[2] = '\0'; t.tm_mon  = atoi(buf) - 1;
    memcpy(buf, dtm + 6, 2); buf[2] = '\0'; t.tm_mday = atoi(buf);

    size_t len = strlen(dtm);
    if (len >= 10) { memcpy(buf, dtm + 8,  2); buf[2]='\0'; t.tm_hour = atoi(buf); }
    if (len >= 12) { memcpy(buf, dtm + 10, 2); buf[2]='\0'; t.tm_min  = atoi(buf); }
    if (len >= 14) { memcpy(buf, dtm + 12, 2); buf[2]='\0'; t.tm_sec  = atoi(buf); }

    t.tm_isdst = -1;
    time_t result = mktime(&t);
    return (result == (time_t)-1) ? 0 : result;
}

/* -----------------------------------------------------------------------
 * Mutation
 * ----------------------------------------------------------------------- */

int hl7_index_add(hl7_channel_index_t *idx, const hl7_message_t *msg) {
    if (!idx || !msg) return -1;

    int rc = 0;
    const char *val;

    /* PID[3] */
    val = hl7_message_get_field(msg, "PID-3");
    if (val && *val) rc |= hash_index_add(idx->by_pid3, val, msg->msg_id);

    /* MSH[9] */
    val = hl7_message_get_field(msg, "MSH-9");
    if (val && *val) rc |= hash_index_add(idx->by_msh9, val, msg->msg_id);

    /* MSH[3] */
    val = hl7_message_get_field(msg, "MSH-3");
    if (val && *val) rc |= hash_index_add(idx->by_msh3, val, msg->msg_id);

    /* MSH[10] */
    val = hl7_message_get_field(msg, "MSH-10");
    if (val && *val) rc |= hash_index_add(idx->by_msh10, val, msg->msg_id);

    /* PV1[19] */
    val = hl7_message_get_field(msg, "PV1-19");
    if (val && *val) rc |= hash_index_add(idx->by_pv119, val, msg->msg_id);

    /* MSH[7] - timestamp */
    val = hl7_message_get_field(msg, "MSH-7");
    if (val && *val) {
        time_t ts = hl7_parse_dtm(val);
        if (ts != 0) {
            if (ts_index_insert(idx->by_msh7, ts, msg->msg_id) != 0) rc = -1;
        }
    }

    if (rc != 0) {
        LOG_WARN("Partial index update failure for msg_id %llu",
                 (unsigned long long)msg->msg_id);
    }
    return rc;
}

void hl7_index_remove(hl7_channel_index_t *idx, const hl7_message_t *msg) {
    if (!idx || !msg) return;

    const char *val;

    val = hl7_message_get_field(msg, "PID-3");
    if (val && *val) hash_index_remove(idx->by_pid3, val, msg->msg_id);

    val = hl7_message_get_field(msg, "MSH-9");
    if (val && *val) hash_index_remove(idx->by_msh9, val, msg->msg_id);

    val = hl7_message_get_field(msg, "MSH-3");
    if (val && *val) hash_index_remove(idx->by_msh3, val, msg->msg_id);

    val = hl7_message_get_field(msg, "MSH-10");
    if (val && *val) hash_index_remove(idx->by_msh10, val, msg->msg_id);

    val = hl7_message_get_field(msg, "PV1-19");
    if (val && *val) hash_index_remove(idx->by_pv119, val, msg->msg_id);

    /* Timestamp index - remove by msg_id */
    ts_index_remove(idx->by_msh7, msg->msg_id);
}

/* -----------------------------------------------------------------------
 * Lookup
 * ----------------------------------------------------------------------- */

static hashmap_t *pick_hash_map(const hl7_channel_index_t *idx,
                                 hl7_index_field_t field) {
    switch (field) {
        case HL7_IDX_FIELD_PID3:  return idx->by_pid3;
        case HL7_IDX_FIELD_MSH9:  return idx->by_msh9;
        case HL7_IDX_FIELD_MSH3:  return idx->by_msh3;
        case HL7_IDX_FIELD_MSH10: return idx->by_msh10;
        case HL7_IDX_FIELD_PV119: return idx->by_pv119;
        default:                   return NULL;
    }
}

const list_t *hl7_index_lookup_eq(const hl7_channel_index_t *idx,
                                    hl7_index_field_t field,
                                    const char *value) {
    if (!idx || !value) return NULL;

    hashmap_t *map = pick_hash_map(idx, field);
    if (!map) return NULL;

    return (const list_t *)hashmap_get(map, value);
}

list_t *hl7_index_lookup_ts_range(const hl7_channel_index_t *idx,
                                    time_t lower_ts,
                                    time_t upper_ts) {
    if (!idx || !idx->by_msh7) return NULL;

    hl7_ts_index_t *t = idx->by_msh7;
    if (t->count == 0) return NULL;

    /* Binary search for lower bound */
    size_t lo = 0, hi = t->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (t->entries[mid].ts < lower_ts) lo = mid + 1;
        else                               hi = mid;
    }
    size_t start = lo;

    /* Binary search for upper bound */
    lo = start; hi = t->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (t->entries[mid].ts <= upper_ts) lo = mid + 1;
        else                                hi = mid;
    }
    size_t end = lo; /* exclusive */

    if (start >= end) return NULL;

    /* Build result list with heap-allocated msg_id copies */
    list_t *result = list_create();
    if (!result) return NULL;

    for (size_t i = start; i < end; i++) {
        uint64_t *id = malloc(sizeof(uint64_t));
        if (!id) {
            /* Free what we allocated so far */
            list_node_t *node = result->head;
            while (node) { free(node->data); node = node->next; }
            list_destroy(result, NULL);
            return NULL;
        }
        *id = t->entries[i].msg_id;
        list_append(result, id);
    }

    return result;
}
