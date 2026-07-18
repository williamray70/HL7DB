/**
 * config.h - Configuration file parser for HL7DB
 *
 * Parses INI-style configuration files with sections and key-value pairs.
 */

#ifndef HL7DB_CONFIG_H
#define HL7DB_CONFIG_H

#include <stdint.h>

/* Configuration structure (opaque) */
typedef struct config config_t;

/**
 * Create a new empty configuration
 *
 * @return Pointer to new configuration, or NULL on failure
 */
config_t *config_create(void);

/**
 * Load configuration from file
 *
 * @param filepath Path to configuration file
 * @return Pointer to configuration, or NULL on failure
 */
config_t *config_load(const char *filepath);

/**
 * Destroy configuration and free resources
 *
 * @param config Configuration to destroy
 */
void config_destroy(config_t *config);

/**
 * Get string value
 *
 * @param config Configuration
 * @param section Section name
 * @param key Key name
 * @param default_value Default value if not found
 * @return Value string, or default_value if not found
 */
const char *config_get_string(const config_t *config,
                               const char *section,
                               const char *key,
                               const char *default_value);

/**
 * Get integer value
 *
 * @param config Configuration
 * @param section Section name
 * @param key Key name
 * @param default_value Default value if not found
 * @return Integer value, or default_value if not found/invalid
 */
int64_t config_get_int(const config_t *config,
                        const char *section,
                        const char *key,
                        int64_t default_value);

/**
 * Get boolean value
 *
 * @param config Configuration
 * @param section Section name
 * @param key Key name
 * @param default_value Default value if not found
 * @return Boolean value (1 or 0), or default_value if not found
 *
 * Recognized true values: "true", "yes", "on", "1"
 * Recognized false values: "false", "no", "off", "0"
 */
int config_get_bool(const config_t *config,
                    const char *section,
                    const char *key,
                    int default_value);

/**
 * Get floating point value
 *
 * @param config Configuration
 * @param section Section name
 * @param key Key name
 * @param default_value Default value if not found
 * @return Double value, or default_value if not found/invalid
 */
double config_get_double(const config_t *config,
                         const char *section,
                         const char *key,
                         double default_value);

/**
 * Set string value
 *
 * @param config Configuration
 * @param section Section name
 * @param key Key name
 * @param value Value to set
 * @return 0 on success, -1 on failure
 */
int config_set_string(config_t *config,
                      const char *section,
                      const char *key,
                      const char *value);

/**
 * Set integer value
 *
 * @param config Configuration
 * @param section Section name
 * @param key Key name
 * @param value Value to set
 * @return 0 on success, -1 on failure
 */
int config_set_int(config_t *config,
                   const char *section,
                   const char *key,
                   int64_t value);

/**
 * Set boolean value
 *
 * @param config Configuration
 * @param section Section name
 * @param key Key name
 * @param value Value to set (non-zero = true)
 * @return 0 on success, -1 on failure
 */
int config_set_bool(config_t *config,
                    const char *section,
                    const char *key,
                    int value);

/**
 * Check if key exists
 *
 * @param config Configuration
 * @param section Section name
 * @param key Key name
 * @return 1 if exists, 0 otherwise
 */
int config_has_key(const config_t *config,
                   const char *section,
                   const char *key);

/**
 * Check if section exists
 *
 * @param config Configuration
 * @param section Section name
 * @return 1 if exists, 0 otherwise
 */
int config_has_section(const config_t *config, const char *section);

/**
 * Save configuration to file
 *
 * @param config Configuration
 * @param filepath Path to save to
 * @return 0 on success, -1 on failure
 */
int config_save(const config_t *config, const char *filepath);

#endif /* HL7DB_CONFIG_H */
