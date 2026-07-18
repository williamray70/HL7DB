/**
 * hl7_message.h - HL7 Message Manipulation Functions
 *
 * Functions for creating, destroying, and querying HL7 message structures.
 */

#ifndef HL7DB_HL7_MESSAGE_H
#define HL7DB_HL7_MESSAGE_H

#include "hl7/hl7_types.h"
#include <stddef.h>

/**
 * Create a new HL7 message structure
 *
 * @return Pointer to new message, or NULL on failure
 */
hl7_message_t *hl7_message_create(void);

/**
 * Destroy HL7 message and free all resources
 *
 * @param msg Message to destroy
 */
void hl7_message_destroy(hl7_message_t *msg);

/**
 * Get segment by ID
 *
 * @param msg Message
 * @param segment_id Segment ID (e.g., "PID", "OBX")
 * @param index Index for repeating segments (0 = first)
 * @return Pointer to segment, or NULL if not found
 */
hl7_segment_t *hl7_message_get_segment(const hl7_message_t *msg,
                                        const char *segment_id,
                                        int index);

/**
 * Get all segments with given ID
 *
 * @param msg Message
 * @param segment_id Segment ID
 * @param count Pointer to store count (can be NULL)
 * @return Array of segment pointers (caller must free), or NULL if none found
 */
hl7_segment_t **hl7_message_get_all_segments(const hl7_message_t *msg,
                                              const char *segment_id,
                                              size_t *count);

/**
 * Get field value from message using path notation
 *
 * @param msg Message
 * @param path Field path (e.g., "PID-3" or "OBX-5.1")
 * @return Field value string, or NULL if not found
 *
 * Path format: SEGMENT-field[.component[.subcomponent]][repetition]
 * Examples: "PID-3", "PID-5.1", "PID-5.1.2", "OBX-5[1]"
 */
const char *hl7_message_get_field(const hl7_message_t *msg, const char *path);

/**
 * Get field value using parsed path
 *
 * @param msg Message
 * @param path Parsed field path structure
 * @return Field value string, or NULL if not found
 */
const char *hl7_message_get_field_by_path(const hl7_message_t *msg,
                                           const hl7_field_path_t *path);

/**
 * Parse field path string into structure
 *
 * @param path_str Path string (e.g., "PID-3.1")
 * @param path Pointer to path structure to fill
 * @return 0 on success, -1 on failure
 */
int hl7_parse_field_path(const char *path_str, hl7_field_path_t *path);

/**
 * Add segment to message
 *
 * @param msg Message
 * @param segment Segment to add
 * @return 0 on success, -1 on failure
 */
int hl7_message_add_segment(hl7_message_t *msg, hl7_segment_t *segment);

/**
 * Create a new segment
 *
 * @param msg Message (for memory pool allocation)
 * @param segment_id Segment ID (e.g., "PID")
 * @param num_fields Number of fields
 * @return Pointer to new segment, or NULL on failure
 */
hl7_segment_t *hl7_segment_create(hl7_message_t *msg,
                                   const char *segment_id,
                                   size_t num_fields);

/**
 * Get field from segment
 *
 * @param segment Segment
 * @param field_index Field index (1-based for HL7 compatibility)
 * @return Pointer to field, or NULL if out of bounds
 */
hl7_field_t *hl7_segment_get_field(const hl7_segment_t *segment, int field_index);

/**
 * Get field value as string
 *
 * @param field Field
 * @return Field value string (may be component concatenated)
 */
const char *hl7_field_get_value(const hl7_field_t *field);

/**
 * Get component from field
 *
 * @param field Field
 * @param component_index Component index (0-based)
 * @return Pointer to component, or NULL if out of bounds
 */
hl7_component_t *hl7_field_get_component(const hl7_field_t *field,
                                          int component_index);

/**
 * Get component value as string
 *
 * @param component Component
 * @return Component value string
 */
const char *hl7_component_get_value(const hl7_component_t *component);

/**
 * Get subcomponent from component
 *
 * @param component Component
 * @param subcomponent_index Subcomponent index (0-based)
 * @return Pointer to subcomponent, or NULL if out of bounds
 */
hl7_subcomponent_t *hl7_component_get_subcomponent(const hl7_component_t *component,
                                                     int subcomponent_index);

/**
 * Get subcomponent value
 *
 * @param subcomponent Subcomponent
 * @return Subcomponent value string
 */
const char *hl7_subcomponent_get_value(const hl7_subcomponent_t *subcomponent);

/**
 * Get message type enum from string
 *
 * @param msg_type_str Message type string (e.g., "ADT^A01")
 * @return Message type enum
 */
hl7_message_type_t hl7_get_message_type(const char *msg_type_str);

/**
 * Get message type string from enum
 *
 * @param msg_type Message type enum
 * @return Message type string (e.g., "ADT"), or "UNKNOWN"
 */
const char *hl7_message_type_to_string(hl7_message_type_t msg_type);

/**
 * Get HL7 version enum from string
 *
 * @param version_str Version string (e.g., "2.5")
 * @return Version enum
 */
hl7_version_t hl7_get_version(const char *version_str);

/**
 * Get version string from enum
 *
 * @param version Version enum
 * @return Version string (e.g., "2.5"), or "unknown"
 */
const char *hl7_version_to_string(hl7_version_t version);

/**
 * Print message structure for debugging
 *
 * @param msg Message
 * @param verbose Include field values
 */
void hl7_message_print(const hl7_message_t *msg, int verbose);

/**
 * Print message structure with PHI redaction (HIPAA-compliant)
 *
 * Safe for use in logs, console output, and non-secure contexts.
 * Redacts Protected Health Information according to HIPAA requirements.
 *
 * @param msg Message
 * @param verbose Include field values (redacted)
 * @param config Redaction configuration (NULL = use default)
 */
void hl7_message_print_redacted(const hl7_message_t *msg,
                                 int verbose,
                                 const void *config);

#endif /* HL7DB_HL7_MESSAGE_H */
