/**
 * persistence.c - Simple Persistence Implementation
 *
 * HIPAA Compliance:
 * - Encryption at rest using AES-256-GCM (45 CFR §164.312(a)(2)(iv))
 * - Restrictive file permissions (0600 for PHI files, 0700 for directories)
 */

#define _POSIX_C_SOURCE 200809L

#include "storage/persistence.h"
#include "security/encryption.h"
#include "hl7/hl7_parser.h"
#include "util/logger.h"
#include "util/base64.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

static persistence_config_t *global_config = NULL;

/**
 * Create directory recursively
 */
static int mkdir_recursive(const char *path) {
    char *path_copy = strdup(path);
    if (!path_copy) return -1;

    char *p = path_copy;

    /* Skip leading slash */
    if (*p == '/') p++;

    while (*p) {
        /* Find next slash */
        while (*p && *p != '/') p++;

        char saved = *p;
        *p = '\0';

        /* Create directory with restrictive permissions (0700 for PHI protection) */
        if (mkdir(path_copy, 0700) != 0 && errno != EEXIST) {
            free(path_copy);
            return -1;
        }

        *p = saved;
        if (*p) p++;
    }

    free(path_copy);
    return 0;
}

int persistence_init(persistence_config_t *config) {
    if (!config || !config->data_dir) {
        LOG_ERROR("Invalid persistence configuration");
        return -1;
    }

    global_config = config;

    /* Create base data directory */
    if (mkdir_recursive(config->data_dir) != 0) {
        LOG_ERROR("Failed to create data directory: %s", config->data_dir);
        return -1;
    }

    LOG_INFO("Persistence initialized: data_dir=%s, sync=%d",
             config->data_dir, config->sync_on_write);

    return 0;
}

void persistence_cleanup(void) {
    global_config = NULL;
}

channel_persistence_t *persistence_open_channel(const char *channel_name) {
    if (!global_config) {
        LOG_ERROR("Persistence not initialized");
        return NULL;
    }

    if (!channel_name || !*channel_name) {
        LOG_ERROR("Invalid channel name");
        return NULL;
    }

    channel_persistence_t *cp = calloc(1, sizeof(channel_persistence_t));
    if (!cp) {
        LOG_ERROR("Failed to allocate channel persistence");
        return NULL;
    }

    cp->channel_name = strdup(channel_name);
    cp->config = global_config;

    /* Build file path: {data_dir}/channels/{name}/messages.dat */
    char channel_dir[2048];
    int ret = snprintf(channel_dir, sizeof(channel_dir), "%s/channels/%s",
                       global_config->data_dir, channel_name);
    if (ret < 0 || (size_t)ret >= sizeof(channel_dir)) {
        LOG_ERROR("Channel directory path too long");
        free(cp->channel_name);
        free(cp);
        return NULL;
    }

    /* Create channel directory */
    if (mkdir_recursive(channel_dir) != 0) {
        LOG_ERROR("Failed to create channel directory: %s", channel_dir);
        free(cp->channel_name);
        free(cp);
        return NULL;
    }

    /* Build messages.dat path */
    char file_path[2048];
    ret = snprintf(file_path, sizeof(file_path), "%s/messages.dat", channel_dir);
    if (ret < 0 || (size_t)ret >= sizeof(file_path)) {
        LOG_ERROR("File path too long");
        free(cp->channel_name);
        free(cp);
        return NULL;
    }
    cp->file_path = strdup(file_path);

    /* Open file for appending (create if doesn't exist) */
    cp->append_file = fopen(cp->file_path, "a");
    if (!cp->append_file) {
        LOG_ERROR("Failed to open persistence file: %s - %s",
                  cp->file_path, strerror(errno));
        free(cp->channel_name);
        free(cp->file_path);
        free(cp);
        return NULL;
    }

    /* Set file permissions to 0600 (owner read/write only) for PHI protection */
    if (chmod(cp->file_path, 0600) != 0) {
        LOG_WARN("Failed to set file permissions to 0600 for %s: %s",
                 cp->file_path, strerror(errno));
    }

    LOG_DEBUG("Opened persistence for channel '%s': %s",
              channel_name, file_path);

    return cp;
}

void persistence_close_channel(channel_persistence_t *cp) {
    if (!cp) return;

    if (cp->append_file) {
        fflush(cp->append_file);
        fclose(cp->append_file);
    }

    free(cp->channel_name);
    free(cp->file_path);
    free(cp);
}

int persistence_append_message(channel_persistence_t *cp,
                                uint64_t msg_id,
                                const hl7_message_t *message) {
    if (!cp || !cp->append_file || !message) {
        return -1;
    }

    /* Get raw message */
    if (!message->raw_message || !*message->raw_message) {
        LOG_WARN("Message %lu has no raw content, skipping persistence", msg_id);
        return -1;
    }

    int written = -1;

    /* Check if encryption is enabled */
    if (cp->config->encrypt_at_rest && encryption_is_enabled()) {
        /* ENCRYPTED PATH - HIPAA Compliant */

        /* Encrypt the message using AES-256-GCM */
        encrypted_data_t encrypted = {0};
        if (encryption_encrypt_message(message->raw_message, msg_id, &encrypted) != 0) {
            LOG_ERROR("Failed to encrypt message %lu", msg_id);
            return -1;
        }

        /* Serialize encrypted data to binary format */
        unsigned char buffer[16384];  /* 16KB buffer for encrypted data */
        size_t buffer_len = sizeof(buffer);
        if (encryption_serialize(&encrypted, buffer, &buffer_len) != 0) {
            LOG_ERROR("Failed to serialize encrypted message %lu", msg_id);
            encryption_free_data(&encrypted);
            return -1;
        }

        /* Encode binary encrypted data as Base64 for text storage */
        size_t b64_len = base64_encode_size(buffer_len);
        char *b64_data = malloc(b64_len);
        if (!b64_data) {
            LOG_ERROR("Failed to allocate Base64 buffer");
            encryption_free_data(&encrypted);
            return -1;
        }

        if (base64_encode(buffer, buffer_len, b64_data, b64_len) < 0) {
            LOG_ERROR("Failed to Base64 encode encrypted message %lu", msg_id);
            free(b64_data);
            encryption_free_data(&encrypted);
            return -1;
        }

        /* Write encrypted message: ENC|msg_id|timestamp|base64_encrypted_data\n */
        written = fprintf(cp->append_file, "ENC|%lu|%lu|%s\n",
                          msg_id,
                          (uint64_t)message->timestamp,
                          b64_data);

        free(b64_data);
        encryption_free_data(&encrypted);

        if (written < 0) {
            LOG_ERROR("Failed to write encrypted message: %s", strerror(errno));
            return -1;
        }

        LOG_DEBUG("Persisted ENCRYPTED message %lu to channel '%s'", msg_id, cp->channel_name);

    } else {
        /* PLAINTEXT PATH - For backward compatibility or when encryption disabled */
        LOG_WARN("Storing message %lu in PLAINTEXT (encryption disabled) - NOT HIPAA COMPLIANT!", msg_id);

        /* Escape special characters: \r -> \\r, \n -> \\n, | -> \\| */
        size_t raw_len = strlen(message->raw_message);
        char *escaped = malloc(raw_len * 2 + 1);
        if (!escaped) {
            LOG_ERROR("Failed to allocate escape buffer");
            return -1;
        }

        char *dst = escaped;
        const char *src = message->raw_message;
        while (*src) {
            if (*src == '\r') {
                *dst++ = '\\';
                *dst++ = 'r';
            } else if (*src == '\n') {
                *dst++ = '\\';
                *dst++ = 'n';
            } else if (*src == '|') {
                *dst++ = '\\';
                *dst++ = '|';
            } else if (*src == '\\') {
                *dst++ = '\\';
                *dst++ = '\\';
            } else {
                *dst++ = *src;
            }
            src++;
        }
        *dst = '\0';

        /* Write plaintext: PLN|msg_id|timestamp|escaped_message\n */
        written = fprintf(cp->append_file, "PLN|%lu|%lu|%s\n",
                          msg_id,
                          (uint64_t)message->timestamp,
                          escaped);

        free(escaped);

        if (written < 0) {
            LOG_ERROR("Failed to write plaintext message: %s", strerror(errno));
            return -1;
        }

        LOG_DEBUG("Persisted PLAINTEXT message %lu to channel '%s'", msg_id, cp->channel_name);
    }

    /* Flush and optionally sync */
    fflush(cp->append_file);
    if (cp->config->sync_on_write) {
        fsync(fileno(cp->append_file));
    }

    return 0;
}

list_t *persistence_load_messages(channel_persistence_t *cp) {
    if (!cp || !cp->file_path) {
        return NULL;
    }

    /* Open file for reading */
    FILE *f = fopen(cp->file_path, "r");
    if (!f) {
        if (errno == ENOENT) {
            /* File doesn't exist yet - return empty list */
            LOG_INFO("No persistence file found for channel '%s', starting empty",
                     cp->channel_name);
            return list_create();
        }
        LOG_ERROR("Failed to open persistence file for reading: %s - %s",
                  cp->file_path, strerror(errno));
        return NULL;
    }

    list_t *messages = list_create();
    if (!messages) {
        fclose(f);
        return NULL;
    }

    /* Read line by line */
    char *line = NULL;
    size_t line_size = 0;
    ssize_t read;
    uint64_t loaded_count = 0;

    while ((read = getline(&line, &line_size, f)) != -1) {
        /* Remove trailing newline */
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }

        /* Check format: ENC or PLN prefix */
        bool is_encrypted = false;
        char *data_start = line;

        if (strncmp(line, "ENC|", 4) == 0) {
            is_encrypted = true;
            data_start = line + 4;
        } else if (strncmp(line, "PLN|", 4) == 0) {
            is_encrypted = false;
            data_start = line + 4;
        } else {
            /* Old format without prefix - assume plaintext */
            is_encrypted = false;
            data_start = line;
        }

        /* Parse: msg_id|timestamp|data */
        char *pipe1 = strchr(data_start, '|');
        if (!pipe1) continue;  /* Malformed line */

        char *pipe2 = strchr(pipe1 + 1, '|');
        if (!pipe2) continue;  /* Malformed line */

        *pipe1 = '\0';
        *pipe2 = '\0';

        uint64_t msg_id = strtoull(data_start, NULL, 10);
        uint64_t timestamp = strtoull(pipe1 + 1, NULL, 10);
        char *message_data = pipe2 + 1;

        char *raw_message = NULL;

        if (is_encrypted) {
            /* DECRYPT ENCRYPTED MESSAGE */

            /* Decode Base64 */
            size_t b64_len = strlen(message_data);
            size_t bin_len = base64_decode_size(b64_len);
            unsigned char *bin_data = malloc(bin_len);
            if (!bin_data) {
                LOG_ERROR("Failed to allocate buffer for decryption");
                continue;
            }

            if (base64_decode(message_data, b64_len, bin_data, &bin_len) != 0) {
                LOG_ERROR("Failed to Base64 decode message %lu", msg_id);
                free(bin_data);
                continue;
            }

            /* Deserialize encrypted data */
            encrypted_data_t encrypted = {0};
            if (encryption_deserialize(bin_data, bin_len, &encrypted) != 0) {
                LOG_ERROR("Failed to deserialize encrypted message %lu", msg_id);
                free(bin_data);
                continue;
            }
            free(bin_data);

            /* Decrypt message */
            char decrypted[65536];  /* 64KB buffer for decrypted message */
            size_t decrypted_len = sizeof(decrypted);

            if (encryption_decrypt_message(&encrypted, msg_id, decrypted, &decrypted_len) != 0) {
                LOG_ERROR("Failed to decrypt message %lu - possible corruption or wrong key", msg_id);
                encryption_free_data(&encrypted);
                continue;
            }

            encryption_free_data(&encrypted);

            raw_message = strndup(decrypted, decrypted_len);
            if (!raw_message) {
                LOG_ERROR("Failed to allocate decrypted message buffer");
                continue;
            }

        } else {
            /* UNESCAPE PLAINTEXT MESSAGE */

            /* Unescape message: \\r -> \r, \\n -> \n, \\| -> |, \\\\ -> \ */
            char *unescaped = malloc(strlen(message_data) + 1);
            if (!unescaped) continue;

            char *dst = unescaped;
            char *src = message_data;
            while (*src) {
                if (*src == '\\' && *(src + 1)) {
                    src++;
                    if (*src == 'r') *dst++ = '\r';
                    else if (*src == 'n') *dst++ = '\n';
                    else if (*src == '|') *dst++ = '|';
                    else if (*src == '\\') *dst++ = '\\';
                    else {
                        *dst++ = '\\';
                        *dst++ = *src;
                    }
                    src++;
                } else {
                    *dst++ = *src++;
                }
            }
            *dst = '\0';

            raw_message = unescaped;
        }

        /* Parse HL7 message */
        hl7_message_t *msg = hl7_parse(raw_message, NULL);
        free(raw_message);

        if (!msg) {
            LOG_WARN("Failed to parse persisted message ID %lu", msg_id);
            continue;
        }

        /* Set message ID and timestamp */
        msg->msg_id = msg_id;
        msg->timestamp = timestamp;

        list_append(messages, msg);
        loaded_count++;
    }

    free(line);
    fclose(f);

    LOG_INFO("Loaded %lu messages from persistence for channel '%s'",
             loaded_count, cp->channel_name);

    return messages;
}

int persistence_rewrite_messages(channel_persistence_t *cp,
                                  const list_t *messages) {
    if (!cp || !cp->file_path || !messages) {
        LOG_ERROR("Invalid arguments to persistence_rewrite_messages");
        return -1;
    }

    /* Build temporary file path: {file_path}.tmp */
    char temp_path[2048];
    int ret = snprintf(temp_path, sizeof(temp_path), "%s.tmp", cp->file_path);
    if (ret < 0 || (size_t)ret >= sizeof(temp_path)) {
        LOG_ERROR("Temporary file path too long");
        return -1;
    }

    /* Open temporary file for writing */
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        LOG_ERROR("Failed to open temporary file for writing: %s - %s",
                  temp_path, strerror(errno));
        return -1;
    }

    /* Write all messages to temporary file */
    uint64_t written_count = 0;
    list_node_t *node = messages->head;
    while (node) {
        hl7_message_t *msg = (hl7_message_t *)node->data;
        if (!msg || !msg->raw_message || !*msg->raw_message) {
            node = node->next;
            continue;
        }

        /* Escape special characters: \r -> \\r, \n -> \\n, | -> \\|, \ -> \\ */
        size_t raw_len = strlen(msg->raw_message);
        char *escaped = malloc(raw_len * 2 + 1);  /* Worst case: all chars need escaping */
        if (!escaped) {
            LOG_ERROR("Failed to allocate escape buffer during rewrite");
            fclose(temp_file);
            unlink(temp_path);
            return -1;
        }

        char *dst = escaped;
        const char *src = msg->raw_message;
        while (*src) {
            if (*src == '\r') {
                *dst++ = '\\';
                *dst++ = 'r';
            } else if (*src == '\n') {
                *dst++ = '\\';
                *dst++ = 'n';
            } else if (*src == '|') {
                *dst++ = '\\';
                *dst++ = '|';
            } else if (*src == '\\') {
                *dst++ = '\\';
                *dst++ = '\\';
            } else {
                *dst++ = *src;
            }
            src++;
        }
        *dst = '\0';

        /* Write: msg_id|timestamp|escaped_message\n */
        int write_ret = fprintf(temp_file, "%lu|%lu|%s\n",
                               msg->msg_id,
                               (uint64_t)msg->timestamp,
                               escaped);

        free(escaped);

        if (write_ret < 0) {
            LOG_ERROR("Failed to write to temporary file during rewrite: %s",
                      strerror(errno));
            fclose(temp_file);
            unlink(temp_path);
            return -1;
        }

        written_count++;
        node = node->next;
    }

    /* Flush and optionally sync */
    if (fflush(temp_file) != 0) {
        LOG_ERROR("Failed to flush temporary file: %s", strerror(errno));
        fclose(temp_file);
        unlink(temp_path);
        return -1;
    }

    if (cp->config->sync_on_write) {
        if (fsync(fileno(temp_file)) != 0) {
            LOG_ERROR("Failed to fsync temporary file: %s", strerror(errno));
            fclose(temp_file);
            unlink(temp_path);
            return -1;
        }
    }

    fclose(temp_file);

    /* Atomically rename temporary file over original */
    if (rename(temp_path, cp->file_path) != 0) {
        LOG_ERROR("Failed to rename temporary file to original: %s - %s",
                  cp->file_path, strerror(errno));
        unlink(temp_path);
        return -1;
    }

    /* Reopen append file handle to ensure it points to the new file */
    if (cp->append_file) {
        fclose(cp->append_file);
    }
    cp->append_file = fopen(cp->file_path, "a");
    if (!cp->append_file) {
        LOG_ERROR("Failed to reopen append file after rewrite: %s - %s",
                  cp->file_path, strerror(errno));
        return -1;
    }

    LOG_DEBUG("Rewrote persistence file for channel '%s': %lu messages",
              cp->channel_name, written_count);

    return 0;
}

int persistence_get_stats(const char *channel_name,
                          uint64_t *message_count,
                          uint64_t *file_size) {
    if (!global_config || !channel_name) {
        return -1;
    }

    char file_path[2048];
    int ret = snprintf(file_path, sizeof(file_path), "%s/channels/%s/messages.dat",
                       global_config->data_dir, channel_name);
    if (ret < 0 || (size_t)ret >= sizeof(file_path)) {
        return -1;
    }

    struct stat st;
    if (stat(file_path, &st) != 0) {
        if (errno == ENOENT) {
            /* File doesn't exist */
            if (message_count) *message_count = 0;
            if (file_size) *file_size = 0;
            return 0;
        }
        return -1;
    }

    if (file_size) {
        *file_size = st.st_size;
    }

    /* Count messages by reading file */
    if (message_count) {
        FILE *f = fopen(file_path, "r");
        if (!f) return -1;

        uint64_t count = 0;
        char *line = NULL;
        size_t line_size = 0;
        while (getline(&line, &line_size, f) != -1) {
            count++;
        }
        free(line);
        fclose(f);

        *message_count = count;
    }

    return 0;
}

/**
 * Get registry file path
 */
static int get_registry_path(char *buf, size_t buf_size) {
    if (!global_config || !global_config->data_dir) {
        return -1;
    }

    int ret = snprintf(buf, buf_size, "%s/channels/channels.dat",
                       global_config->data_dir);
    if (ret < 0 || (size_t)ret >= buf_size) {
        return -1;
    }
    return 0;
}

int persistence_register_channel(const char *channel_name) {
    if (!global_config) {
        LOG_ERROR("Persistence not initialized");
        return -1;
    }

    if (!channel_name || !*channel_name) {
        LOG_ERROR("Invalid channel name");
        return -1;
    }

    /* Get registry file path */
    char registry_path[2048];
    if (get_registry_path(registry_path, sizeof(registry_path)) != 0) {
        LOG_ERROR("Registry path too long");
        return -1;
    }

    /* Ensure channels directory exists */
    char channels_dir[2048];
    int ret = snprintf(channels_dir, sizeof(channels_dir), "%s/channels",
                       global_config->data_dir);
    if (ret < 0 || (size_t)ret >= sizeof(channels_dir)) {
        LOG_ERROR("Channels directory path too long");
        return -1;
    }

    if (mkdir_recursive(channels_dir) != 0) {
        LOG_ERROR("Failed to create channels directory: %s", channels_dir);
        return -1;
    }

    /* Append channel name to registry */
    FILE *f = fopen(registry_path, "a");
    if (!f) {
        LOG_ERROR("Failed to open registry file for appending: %s - %s",
                  registry_path, strerror(errno));
        return -1;
    }

    if (fprintf(f, "%s\n", channel_name) < 0) {
        LOG_ERROR("Failed to write to registry file: %s", strerror(errno));
        fclose(f);
        return -1;
    }

    fflush(f);
    if (global_config->sync_on_write) {
        fsync(fileno(f));
    }
    fclose(f);

    LOG_DEBUG("Registered channel '%s' in registry", channel_name);
    return 0;
}

int persistence_unregister_channel(const char *channel_name) {
    if (!global_config) {
        LOG_ERROR("Persistence not initialized");
        return -1;
    }

    if (!channel_name || !*channel_name) {
        LOG_ERROR("Invalid channel name");
        return -1;
    }

    /* Get registry file path */
    char registry_path[2048];
    if (get_registry_path(registry_path, sizeof(registry_path)) != 0) {
        LOG_ERROR("Registry path too long");
        return -1;
    }

    /* Build temporary file path */
    char temp_path[2048];
    int ret = snprintf(temp_path, sizeof(temp_path), "%s.tmp", registry_path);
    if (ret < 0 || (size_t)ret >= sizeof(temp_path)) {
        LOG_ERROR("Temporary registry path too long");
        return -1;
    }

    /* Open original file for reading */
    FILE *orig = fopen(registry_path, "r");
    if (!orig) {
        if (errno == ENOENT) {
            /* Registry doesn't exist - nothing to remove */
            LOG_DEBUG("Registry file doesn't exist, nothing to unregister");
            return 0;
        }
        LOG_ERROR("Failed to open registry file for reading: %s - %s",
                  registry_path, strerror(errno));
        return -1;
    }

    /* Open temporary file for writing */
    FILE *temp = fopen(temp_path, "w");
    if (!temp) {
        LOG_ERROR("Failed to open temporary registry file for writing: %s - %s",
                  temp_path, strerror(errno));
        fclose(orig);
        return -1;
    }

    /* Copy all lines except the one to remove */
    char *line = NULL;
    size_t line_size = 0;
    ssize_t read;
    int found = 0;

    while ((read = getline(&line, &line_size, orig)) != -1) {
        /* Remove trailing newline */
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        /* Skip the channel to remove */
        if (strcmp(line, channel_name) == 0) {
            found = 1;
            continue;
        }

        /* Write other channels */
        if (fprintf(temp, "%s\n", line) < 0) {
            LOG_ERROR("Failed to write to temporary registry: %s", strerror(errno));
            free(line);
            fclose(temp);
            fclose(orig);
            unlink(temp_path);
            return -1;
        }
    }

    free(line);
    fclose(orig);

    /* Flush and sync */
    if (fflush(temp) != 0) {
        LOG_ERROR("Failed to flush temporary registry: %s", strerror(errno));
        fclose(temp);
        unlink(temp_path);
        return -1;
    }

    if (global_config->sync_on_write) {
        if (fsync(fileno(temp)) != 0) {
            LOG_ERROR("Failed to fsync temporary registry: %s", strerror(errno));
            fclose(temp);
            unlink(temp_path);
            return -1;
        }
    }

    fclose(temp);

    /* Atomically rename */
    if (rename(temp_path, registry_path) != 0) {
        LOG_ERROR("Failed to rename temporary registry: %s - %s",
                  registry_path, strerror(errno));
        unlink(temp_path);
        return -1;
    }

    if (found) {
        LOG_DEBUG("Unregistered channel '%s' from registry", channel_name);
    } else {
        LOG_WARN("Channel '%s' not found in registry", channel_name);
    }

    return 0;
}

char **persistence_load_channel_registry(size_t *count) {
    if (!global_config) {
        LOG_ERROR("Persistence not initialized");
        return NULL;
    }

    if (!count) {
        LOG_ERROR("Invalid count parameter");
        return NULL;
    }

    *count = 0;

    /* Get registry file path */
    char registry_path[2048];
    if (get_registry_path(registry_path, sizeof(registry_path)) != 0) {
        LOG_ERROR("Registry path too long");
        return NULL;
    }

    /* Open registry file */
    FILE *f = fopen(registry_path, "r");
    if (!f) {
        if (errno == ENOENT) {
            /* Registry doesn't exist yet - return empty array */
            LOG_INFO("No registry file found, starting with empty channel list");
            return NULL;
        }
        LOG_ERROR("Failed to open registry file: %s - %s",
                  registry_path, strerror(errno));
        return NULL;
    }

    /* First pass: count lines */
    size_t line_count = 0;
    char *line = NULL;
    size_t line_size = 0;
    while (getline(&line, &line_size, f) != -1) {
        line_count++;
    }
    free(line);

    if (line_count == 0) {
        fclose(f);
        LOG_INFO("Registry file is empty");
        return NULL;
    }

    /* Allocate array for channel names */
    char **names = calloc(line_count, sizeof(char *));
    if (!names) {
        LOG_ERROR("Failed to allocate channel names array");
        fclose(f);
        return NULL;
    }

    /* Rewind and read channel names */
    rewind(f);
    line = NULL;
    line_size = 0;
    size_t idx = 0;

    while (getline(&line, &line_size, f) != -1 && idx < line_count) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        /* Skip empty lines */
        if (line[0] == '\0') {
            continue;
        }

        /* Duplicate channel name */
        names[idx] = strdup(line);
        if (!names[idx]) {
            LOG_ERROR("Failed to duplicate channel name");
            /* Free what we've allocated so far */
            for (size_t i = 0; i < idx; i++) {
                free(names[i]);
            }
            free(names);
            free(line);
            fclose(f);
            return NULL;
        }
        idx++;
    }

    free(line);
    fclose(f);

    *count = idx;
    LOG_INFO("Loaded %zu channels from registry", idx);
    return names;
}

void persistence_free_channel_registry(char **names, size_t count) {
    if (!names) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(names[i]);
    }
    free(names);
}
