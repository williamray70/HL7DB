/**
 * config.c - Configuration file parser implementation
 */

#define _POSIX_C_SOURCE 200809L

#include "util/config.h"
#include "util/hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE_LENGTH 1024

/* Configuration entry */
typedef struct config_entry {
    char *key;
    char *value;
} config_entry_t;

/* Configuration section */
typedef struct config_section {
    char *name;
    hashmap_t *entries;  /* key -> value */
} config_section_t;

/* Configuration structure */
struct config {
    hashmap_t *sections;  /* section_name -> config_section_t */
};

/**
 * Trim whitespace from string
 */
static char *trim(char *str) {
    /* Trim leading whitespace */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    /* Trim trailing whitespace */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    *(end + 1) = '\0';
    return str;
}

/**
 * Free config entry
 */
static void free_entry(void *ptr) {
    config_entry_t *entry = (config_entry_t *)ptr;
    if (entry) {
        free(entry->key);
        free(entry->value);
        free(entry);
    }
}

/**
 * Free config section
 */
static void free_section(void *ptr) {
    config_section_t *section = (config_section_t *)ptr;
    if (section) {
        free(section->name);
        hashmap_destroy(section->entries, free_entry);
        free(section);
    }
}

/**
 * Create a new empty configuration
 */
config_t *config_create(void) {
    config_t *config = malloc(sizeof(config_t));
    if (!config) {
        return NULL;
    }

    config->sections = hashmap_create(16);
    if (!config->sections) {
        free(config);
        return NULL;
    }

    return config;
}

/**
 * Get or create section
 */
static config_section_t *get_or_create_section(config_t *config, const char *section_name) {
    if (!section_name) {
        section_name = "";
    }

    /* Check if section exists */
    config_section_t *section = hashmap_get(config->sections, section_name);
    if (section) {
        return section;
    }

    /* Create new section */
    section = malloc(sizeof(config_section_t));
    if (!section) {
        return NULL;
    }

    section->name = strdup(section_name);
    section->entries = hashmap_create(16);

    if (!section->name || !section->entries) {
        free_section(section);
        return NULL;
    }

    hashmap_put(config->sections, section->name, section);
    return section;
}

/**
 * Load configuration from file
 */
config_t *config_load(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        return NULL;
    }

    config_t *config = config_create();
    if (!config) {
        fclose(file);
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    char *current_section = NULL;
    int line_num = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        /* Trim whitespace */
        char *trimmed = trim(line);

        /* Skip empty lines and comments */
        if (*trimmed == '\0' || *trimmed == '#' || *trimmed == ';') {
            continue;
        }

        /* Check for section header */
        if (*trimmed == '[') {
            char *end = strchr(trimmed, ']');
            if (!end) {
                /* Invalid section header */
                continue;
            }

            *end = '\0';
            free(current_section);
            current_section = strdup(trimmed + 1);
            continue;
        }

        /* Parse key=value */
        char *equals = strchr(trimmed, '=');
        if (!equals) {
            /* Not a valid key=value line */
            continue;
        }

        *equals = '\0';
        char *key = trim(trimmed);
        char *value = trim(equals + 1);

        /* Remove quotes from value if present */
        size_t value_len = strlen(value);
        if (value_len >= 2 &&
            ((value[0] == '"' && value[value_len - 1] == '"') ||
             (value[0] == '\'' && value[value_len - 1] == '\''))) {
            value[value_len - 1] = '\0';
            value++;
        }

        /* Add to configuration */
        const char *section_name = current_section ? current_section : "";
        config_set_string(config, section_name, key, value);
    }

    free(current_section);
    fclose(file);

    return config;
}

/**
 * Destroy configuration and free resources
 */
void config_destroy(config_t *config) {
    if (!config) {
        return;
    }

    hashmap_destroy(config->sections, free_section);
    free(config);
}

/**
 * Get string value
 */
const char *config_get_string(const config_t *config,
                               const char *section,
                               const char *key,
                               const char *default_value) {
    if (!config || !key) {
        return default_value;
    }

    if (!section) {
        section = "";
    }

    config_section_t *sec = hashmap_get(config->sections, section);
    if (!sec) {
        return default_value;
    }

    config_entry_t *entry = hashmap_get(sec->entries, key);
    if (!entry) {
        return default_value;
    }

    return entry->value;
}

/**
 * Get integer value
 */
int64_t config_get_int(const config_t *config,
                        const char *section,
                        const char *key,
                        int64_t default_value) {
    const char *str = config_get_string(config, section, key, NULL);
    if (!str) {
        return default_value;
    }

    char *endptr;
    errno = 0;
    int64_t value = strtoll(str, &endptr, 10);

    if (errno != 0 || *endptr != '\0') {
        return default_value;
    }

    return value;
}

/**
 * Get boolean value
 */
int config_get_bool(const config_t *config,
                    const char *section,
                    const char *key,
                    int default_value) {
    const char *str = config_get_string(config, section, key, NULL);
    if (!str) {
        return default_value;
    }

    /* Check for true values */
    if (strcasecmp(str, "true") == 0 ||
        strcasecmp(str, "yes") == 0 ||
        strcasecmp(str, "on") == 0 ||
        strcmp(str, "1") == 0) {
        return 1;
    }

    /* Check for false values */
    if (strcasecmp(str, "false") == 0 ||
        strcasecmp(str, "no") == 0 ||
        strcasecmp(str, "off") == 0 ||
        strcmp(str, "0") == 0) {
        return 0;
    }

    return default_value;
}

/**
 * Get floating point value
 */
double config_get_double(const config_t *config,
                         const char *section,
                         const char *key,
                         double default_value) {
    const char *str = config_get_string(config, section, key, NULL);
    if (!str) {
        return default_value;
    }

    char *endptr;
    errno = 0;
    double value = strtod(str, &endptr);

    if (errno != 0 || *endptr != '\0') {
        return default_value;
    }

    return value;
}

/**
 * Set string value
 */
int config_set_string(config_t *config,
                      const char *section,
                      const char *key,
                      const char *value) {
    if (!config || !key || !value) {
        return -1;
    }

    config_section_t *sec = get_or_create_section(config, section);
    if (!sec) {
        return -1;
    }

    /* Create or update entry */
    config_entry_t *entry = hashmap_get(sec->entries, key);
    if (entry) {
        /* Update existing entry */
        free(entry->value);
        entry->value = strdup(value);
        if (!entry->value) {
            return -1;
        }
    } else {
        /* Create new entry */
        entry = malloc(sizeof(config_entry_t));
        if (!entry) {
            return -1;
        }

        entry->key = strdup(key);
        entry->value = strdup(value);

        if (!entry->key || !entry->value) {
            free_entry(entry);
            return -1;
        }

        hashmap_put(sec->entries, entry->key, entry);
    }

    return 0;
}

/**
 * Set integer value
 */
int config_set_int(config_t *config,
                   const char *section,
                   const char *key,
                   int64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)value);
    return config_set_string(config, section, key, buf);
}

/**
 * Set boolean value
 */
int config_set_bool(config_t *config,
                    const char *section,
                    const char *key,
                    int value) {
    return config_set_string(config, section, key, value ? "true" : "false");
}

/**
 * Check if key exists
 */
int config_has_key(const config_t *config,
                   const char *section,
                   const char *key) {
    if (!config || !key) {
        return 0;
    }

    if (!section) {
        section = "";
    }

    config_section_t *sec = hashmap_get(config->sections, section);
    if (!sec) {
        return 0;
    }

    return hashmap_contains(sec->entries, key);
}

/**
 * Check if section exists
 */
int config_has_section(const config_t *config, const char *section) {
    if (!config || !section) {
        return 0;
    }

    return hashmap_contains(config->sections, section);
}

/**
 * Save configuration to file
 */
int config_save(const config_t *config, const char *filepath) {
    if (!config || !filepath) {
        return -1;
    }

    FILE *file = fopen(filepath, "w");
    if (!file) {
        return -1;
    }

    /* Iterate over sections */
    hashmap_iter_t sec_iter;
    if (hashmap_iter_init(config->sections, &sec_iter)) {
        do {
            config_section_t *section = hashmap_iter_get_value(&sec_iter);
            if (!section) continue;

            /* Write section header */
            if (section->name && section->name[0] != '\0') {
                fprintf(file, "[%s]\n", section->name);
            }

            /* Write entries */
            hashmap_iter_t entry_iter;
            if (hashmap_iter_init(section->entries, &entry_iter)) {
                do {
                    config_entry_t *entry = hashmap_iter_get_value(&entry_iter);
                    if (entry) {
                        fprintf(file, "%s = %s\n", entry->key, entry->value);
                    }
                } while (hashmap_iter_next(&entry_iter));
            }

            fprintf(file, "\n");
        } while (hashmap_iter_next(&sec_iter));
    }

    fclose(file);
    return 0;
}
