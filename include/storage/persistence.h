/**
 * persistence.h - Simple Persistence Layer for HL7DB
 *
 * Provides append-only message persistence per channel.
 * Format: line-delimited records in {data_dir}/channels/{name}/messages.dat
 */

#ifndef HL7DB_PERSISTENCE_H
#define HL7DB_PERSISTENCE_H

#include "hl7/hl7_message.h"
#include "util/list.h"
#include <stdint.h>
#include <stdio.h>

/**
 * Persistence configuration
 */
typedef struct {
    char *data_dir;              /* Base data directory (e.g., "./data") */
    int sync_on_write;           /* fsync after each write (slower but safer) */
    int encrypt_at_rest;         /* Encrypt PHI data at rest (HIPAA required) */
} persistence_config_t;

/**
 * Channel persistence handle
 */
typedef struct {
    char *channel_name;          /* Channel name */
    char *file_path;             /* Full path to messages.dat */
    FILE *append_file;           /* File handle for appending */
    persistence_config_t *config;
} channel_persistence_t;

/**
 * Initialize persistence system with configuration
 *
 * Args:
 *   config: Persistence configuration
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int persistence_init(persistence_config_t *config);

/**
 * Cleanup persistence system
 */
void persistence_cleanup(void);

/**
 * Open channel for persistence
 *
 * Args:
 *   channel_name: Name of channel
 *
 * Returns:
 *   Channel persistence handle, or NULL on failure
 */
channel_persistence_t *persistence_open_channel(const char *channel_name);

/**
 * Close channel persistence
 *
 * Args:
 *   cp: Channel persistence handle
 */
void persistence_close_channel(channel_persistence_t *cp);

/**
 * Append message to persistent storage
 *
 * Args:
 *   cp: Channel persistence handle
 *   msg_id: Message ID
 *   message: HL7 message
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int persistence_append_message(channel_persistence_t *cp,
                                uint64_t msg_id,
                                const hl7_message_t *message);

/**
 * Load all messages from persistent storage
 *
 * Args:
 *   cp: Channel persistence handle
 *
 * Returns:
 *   List of hl7_message_t* (caller must NOT free messages), or NULL on failure
 */
list_t *persistence_load_messages(channel_persistence_t *cp);

/**
 * Rewrite persistent storage with current message list (atomic)
 *
 * Used after POP operations to remove messages from persistent storage.
 * Writes to a temporary file first, then atomically renames over the original.
 *
 * Args:
 *   cp: Channel persistence handle
 *   messages: List of hl7_message_t* to persist (current state)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int persistence_rewrite_messages(channel_persistence_t *cp,
                                  const list_t *messages);

/**
 * Get persistence statistics
 *
 * Args:
 *   channel_name: Channel name
 *   message_count: Output for message count
 *   file_size: Output for file size in bytes
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int persistence_get_stats(const char *channel_name,
                          uint64_t *message_count,
                          uint64_t *file_size);

/**
 * Append channel name to registry file
 *
 * Adds a channel name to the registry at data/channels/channels.dat.
 *
 * Args:
 *   channel_name: Name of channel to register
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int persistence_register_channel(const char *channel_name);

/**
 * Remove channel name from registry file
 *
 * Atomically rewrites the registry without the specified channel.
 *
 * Args:
 *   channel_name: Name of channel to unregister
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int persistence_unregister_channel(const char *channel_name);

/**
 * Load all registered channel names
 *
 * Reads the registry file and returns an array of channel names.
 * Caller must free the array and individual strings.
 *
 * Args:
 *   count: Output parameter for number of channels
 *
 * Returns:
 *   Array of channel name strings, or NULL on failure
 */
char **persistence_load_channel_registry(size_t *count);

/**
 * Free channel registry array
 *
 * Args:
 *   names: Array of channel names
 *   count: Number of names in array
 */
void persistence_free_channel_registry(char **names, size_t count);

#endif /* HL7DB_PERSISTENCE_H */
