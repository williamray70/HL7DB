/**
 * list.h - Generic doubly-linked list for HL7DB
 *
 * Provides a generic linked list implementation for storing
 * sequences of HL7 segments, fields, and other data.
 */

#ifndef HL7DB_LIST_H
#define HL7DB_LIST_H

#include <stddef.h>

/* List node structure */
typedef struct list_node {
    void *data;                /* Pointer to node data */
    struct list_node *next;    /* Next node in list */
    struct list_node *prev;    /* Previous node in list */
} list_node_t;

/* List structure */
typedef struct list {
    list_node_t *head;         /* First node */
    list_node_t *tail;         /* Last node */
    size_t size;               /* Number of nodes */
} list_t;

/* Function pointer types */
typedef void (*list_free_fn)(void *data);
typedef int (*list_compare_fn)(const void *a, const void *b);
typedef int (*list_foreach_fn)(void *data, void *user_data);

/**
 * Create a new empty list
 *
 * @return Pointer to new list, or NULL on failure
 */
list_t *list_create(void);

/**
 * Destroy list and optionally free data
 *
 * @param list List to destroy
 * @param free_fn Function to free node data (NULL = don't free data)
 */
void list_destroy(list_t *list, list_free_fn free_fn);

/**
 * Get list size
 *
 * @param list List
 * @return Number of nodes in list
 */
size_t list_size(const list_t *list);

/**
 * Check if list is empty
 *
 * @param list List
 * @return 1 if empty, 0 otherwise
 */
int list_is_empty(const list_t *list);

/**
 * Add data to end of list
 *
 * @param list List
 * @param data Data to add
 * @return 0 on success, -1 on failure
 */
int list_append(list_t *list, void *data);

/**
 * Add data to beginning of list
 *
 * @param list List
 * @param data Data to add
 * @return 0 on success, -1 on failure
 */
int list_prepend(list_t *list, void *data);

/**
 * Insert data at specific index
 *
 * @param list List
 * @param index Index to insert at (0 = beginning)
 * @param data Data to insert
 * @return 0 on success, -1 on failure
 */
int list_insert(list_t *list, size_t index, void *data);

/**
 * Remove and return first element
 *
 * @param list List
 * @return Data from first element, or NULL if empty
 */
void *list_remove_first(list_t *list);

/**
 * Remove and return last element
 *
 * @param list List
 * @return Data from last element, or NULL if empty
 */
void *list_remove_last(list_t *list);

/**
 * Remove element at specific index
 *
 * @param list List
 * @param index Index to remove
 * @return Data from removed element, or NULL if not found
 */
void *list_remove_at(list_t *list, size_t index);

/**
 * Remove first occurrence of data
 *
 * @param list List
 * @param data Data to remove
 * @return 1 if removed, 0 if not found
 */
int list_remove(list_t *list, void *data);

/**
 * Get data at specific index
 *
 * @param list List
 * @param index Index to get
 * @return Data at index, or NULL if out of bounds
 */
void *list_get(const list_t *list, size_t index);

/**
 * Get first element data
 *
 * @param list List
 * @return Data from first element, or NULL if empty
 */
void *list_first(const list_t *list);

/**
 * Get last element data
 *
 * @param list List
 * @return Data from last element, or NULL if empty
 */
void *list_last(const list_t *list);

/**
 * Find first occurrence of data
 *
 * @param list List
 * @param data Data to find
 * @return Index of data, or -1 if not found
 */
int list_index_of(const list_t *list, void *data);

/**
 * Check if list contains data
 *
 * @param list List
 * @param data Data to check
 * @return 1 if found, 0 otherwise
 */
int list_contains(const list_t *list, void *data);

/**
 * Clear all elements from list
 *
 * @param list List
 * @param free_fn Function to free node data (NULL = don't free data)
 */
void list_clear(list_t *list, list_free_fn free_fn);

/**
 * Iterate over list elements
 *
 * @param list List
 * @param fn Function to call for each element
 * @param user_data User data to pass to function
 * @return Number of elements processed, or -1 if fn returns non-zero
 */
int list_foreach(list_t *list, list_foreach_fn fn, void *user_data);

/**
 * Convert list to array
 *
 * @param list List
 * @return Array of data pointers (caller must free), or NULL on failure
 *
 * Note: Array is NULL-terminated and has size+1 elements
 */
void **list_to_array(const list_t *list);

#endif /* HL7DB_LIST_H */
