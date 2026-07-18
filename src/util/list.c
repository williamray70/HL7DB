/**
 * list.c - Generic doubly-linked list implementation
 */

#include "util/list.h"
#include <stdlib.h>
#include <string.h>

/**
 * Create a new list node
 */
static list_node_t *create_node(void *data) {
    list_node_t *node = malloc(sizeof(list_node_t));
    if (!node) {
        return NULL;
    }

    node->data = data;
    node->next = NULL;
    node->prev = NULL;

    return node;
}

/**
 * Free a list node
 */
static void free_node(list_node_t *node, list_free_fn free_fn) {
    if (!node) {
        return;
    }

    if (free_fn && node->data) {
        free_fn(node->data);
    }

    free(node);
}

/**
 * Create a new empty list
 */
list_t *list_create(void) {
    list_t *list = malloc(sizeof(list_t));
    if (!list) {
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;

    return list;
}

/**
 * Destroy list and optionally free data
 */
void list_destroy(list_t *list, list_free_fn free_fn) {
    if (!list) {
        return;
    }

    list_clear(list, free_fn);
    free(list);
}

/**
 * Get list size
 */
size_t list_size(const list_t *list) {
    return list ? list->size : 0;
}

/**
 * Check if list is empty
 */
int list_is_empty(const list_t *list) {
    return list ? (list->size == 0) : 1;
}

/**
 * Add data to end of list
 */
int list_append(list_t *list, void *data) {
    if (!list) {
        return -1;
    }

    list_node_t *node = create_node(data);
    if (!node) {
        return -1;
    }

    if (list->tail) {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    } else {
        list->head = list->tail = node;
    }

    list->size++;
    return 0;
}

/**
 * Add data to beginning of list
 */
int list_prepend(list_t *list, void *data) {
    if (!list) {
        return -1;
    }

    list_node_t *node = create_node(data);
    if (!node) {
        return -1;
    }

    if (list->head) {
        list->head->prev = node;
        node->next = list->head;
        list->head = node;
    } else {
        list->head = list->tail = node;
    }

    list->size++;
    return 0;
}

/**
 * Insert data at specific index
 */
int list_insert(list_t *list, size_t index, void *data) {
    if (!list) {
        return -1;
    }

    if (index == 0) {
        return list_prepend(list, data);
    }

    if (index >= list->size) {
        return list_append(list, data);
    }

    /* Find node at index */
    list_node_t *curr = list->head;
    for (size_t i = 0; i < index; i++) {
        curr = curr->next;
    }

    /* Create new node and insert before curr */
    list_node_t *node = create_node(data);
    if (!node) {
        return -1;
    }

    node->next = curr;
    node->prev = curr->prev;
    curr->prev->next = node;
    curr->prev = node;

    list->size++;
    return 0;
}

/**
 * Remove and return first element
 */
void *list_remove_first(list_t *list) {
    if (!list || !list->head) {
        return NULL;
    }

    list_node_t *node = list->head;
    void *data = node->data;

    list->head = node->next;
    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }

    list->size--;
    free(node);

    return data;
}

/**
 * Remove and return last element
 */
void *list_remove_last(list_t *list) {
    if (!list || !list->tail) {
        return NULL;
    }

    list_node_t *node = list->tail;
    void *data = node->data;

    list->tail = node->prev;
    if (list->tail) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }

    list->size--;
    free(node);

    return data;
}

/**
 * Remove element at specific index
 */
void *list_remove_at(list_t *list, size_t index) {
    if (!list || index >= list->size) {
        return NULL;
    }

    if (index == 0) {
        return list_remove_first(list);
    }

    if (index == list->size - 1) {
        return list_remove_last(list);
    }

    /* Find node at index */
    list_node_t *curr = list->head;
    for (size_t i = 0; i < index; i++) {
        curr = curr->next;
    }

    void *data = curr->data;

    /* Remove from list */
    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;

    list->size--;
    free(curr);

    return data;
}

/**
 * Remove first occurrence of data
 */
int list_remove(list_t *list, void *data) {
    if (!list) {
        return 0;
    }

    list_node_t *curr = list->head;
    while (curr) {
        if (curr->data == data) {
            /* Remove this node */
            if (curr->prev) {
                curr->prev->next = curr->next;
            } else {
                list->head = curr->next;
            }

            if (curr->next) {
                curr->next->prev = curr->prev;
            } else {
                list->tail = curr->prev;
            }

            list->size--;
            free(curr);
            return 1;
        }
        curr = curr->next;
    }

    return 0;
}

/**
 * Get data at specific index
 */
void *list_get(const list_t *list, size_t index) {
    if (!list || index >= list->size) {
        return NULL;
    }

    list_node_t *curr = list->head;
    for (size_t i = 0; i < index; i++) {
        curr = curr->next;
    }

    return curr->data;
}

/**
 * Get first element data
 */
void *list_first(const list_t *list) {
    if (!list || !list->head) {
        return NULL;
    }
    return list->head->data;
}

/**
 * Get last element data
 */
void *list_last(const list_t *list) {
    if (!list || !list->tail) {
        return NULL;
    }
    return list->tail->data;
}

/**
 * Find first occurrence of data
 */
int list_index_of(const list_t *list, void *data) {
    if (!list) {
        return -1;
    }

    list_node_t *curr = list->head;
    int index = 0;

    while (curr) {
        if (curr->data == data) {
            return index;
        }
        curr = curr->next;
        index++;
    }

    return -1;
}

/**
 * Check if list contains data
 */
int list_contains(const list_t *list, void *data) {
    return list_index_of(list, data) != -1;
}

/**
 * Clear all elements from list
 */
void list_clear(list_t *list, list_free_fn free_fn) {
    if (!list) {
        return;
    }

    list_node_t *curr = list->head;
    while (curr) {
        list_node_t *next = curr->next;
        free_node(curr, free_fn);
        curr = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

/**
 * Iterate over list elements
 */
int list_foreach(list_t *list, list_foreach_fn fn, void *user_data) {
    if (!list || !fn) {
        return -1;
    }

    list_node_t *curr = list->head;
    int count = 0;

    while (curr) {
        int result = fn(curr->data, user_data);
        if (result != 0) {
            return -1;
        }
        curr = curr->next;
        count++;
    }

    return count;
}

/**
 * Convert list to array
 */
void **list_to_array(const list_t *list) {
    if (!list) {
        return NULL;
    }

    /* Allocate array (NULL-terminated) */
    void **array = malloc(sizeof(void *) * (list->size + 1));
    if (!array) {
        return NULL;
    }

    /* Copy data pointers */
    list_node_t *curr = list->head;
    size_t i = 0;

    while (curr) {
        array[i++] = curr->data;
        curr = curr->next;
    }

    array[i] = NULL;
    return array;
}
