/**
 * hl7_message.c - HL7 Message Manipulation Implementation
 */

#include "hl7/hl7_message.h"
#include "hl7/hl7_types.h"
#include "util/memory.h"
#include "util/logger.h"
#include "util/phi_redaction.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/**
 * Create a new HL7 message structure
 */
hl7_message_t *hl7_message_create(void) {
    hl7_message_t *msg = calloc(1, sizeof(hl7_message_t));
    if (!msg) {
        return NULL;
    }

    /* Create memory pool for message data */
    msg->memory_pool = memory_pool_create(MEMORY_POOL_DEFAULT_SIZE);
    if (!msg->memory_pool) {
        free(msg);
        return NULL;
    }

    /* Set default encoding characters */
    msg->encoding.field_separator = HL7_FIELD_DELIMITER;
    msg->encoding.component_separator = HL7_COMPONENT_DELIMITER;
    msg->encoding.repetition_separator = HL7_REPETITION_DELIMITER;
    msg->encoding.escape_character = HL7_ESCAPE_CHAR;
    msg->encoding.subcomponent_separator = HL7_SUBCOMPONENT_DELIMITER;

    msg->msg_type = HL7_MSG_UNKNOWN;
    msg->version = HL7_VERSION_UNKNOWN;

    return msg;
}

/**
 * Destroy HL7 message and free all resources
 */
void hl7_message_destroy(hl7_message_t *msg) {
    if (!msg) {
        return;
    }

    /* Memory pool owns all allocated strings and structures except the segments list */
    if (msg->memory_pool) {
        memory_pool_destroy(msg->memory_pool);
    }

    free(msg);
}

/**
 * Get segment by ID
 */
hl7_segment_t *hl7_message_get_segment(const hl7_message_t *msg,
                                        const char *segment_id,
                                        int index) {
    if (!msg || !segment_id) {
        return NULL;
    }

    int count = 0;
    hl7_segment_t *seg = msg->segments;

    while (seg) {
        if (strcmp(seg->id, segment_id) == 0) {
            if (count == index) {
                return seg;
            }
            count++;
        }
        seg = seg->next;
    }

    return NULL;
}

/**
 * Get all segments with given ID
 */
hl7_segment_t **hl7_message_get_all_segments(const hl7_message_t *msg,
                                              const char *segment_id,
                                              size_t *count) {
    if (!msg || !segment_id) {
        if (count) *count = 0;
        return NULL;
    }

    /* Count matching segments */
    size_t num_matches = 0;
    hl7_segment_t *seg = msg->segments;
    while (seg) {
        if (strcmp(seg->id, segment_id) == 0) {
            num_matches++;
        }
        seg = seg->next;
    }

    if (num_matches == 0) {
        if (count) *count = 0;
        return NULL;
    }

    /* Allocate array */
    hl7_segment_t **segments = malloc(sizeof(hl7_segment_t *) * num_matches);
    if (!segments) {
        if (count) *count = 0;
        return NULL;
    }

    /* Fill array */
    size_t idx = 0;
    seg = msg->segments;
    while (seg && idx < num_matches) {
        if (strcmp(seg->id, segment_id) == 0) {
            segments[idx++] = seg;
        }
        seg = seg->next;
    }

    if (count) *count = num_matches;
    return segments;
}

/**
 * Parse field path string
 */
int hl7_parse_field_path(const char *path_str, hl7_field_path_t *path) {
    if (!path_str || !path) {
        return -1;
    }

    memset(path, 0, sizeof(hl7_field_path_t));
    path->component_index = -1;
    path->subcomponent_index = -1;
    path->repetition_index = -1;
    path->segment_index = 0;

    /* Parse segment ID (e.g., "PID" from "PID-3.1") */
    const char *dash = strchr(path_str, '-');
    if (!dash) {
        return -1;
    }

    size_t seg_len = dash - path_str;
    if (seg_len > HL7_MAX_SEGMENT_ID_LEN) {
        return -1;
    }

    strncpy(path->segment_id, path_str, seg_len);
    path->segment_id[seg_len] = '\0';

    /* Parse field index */
    const char *ptr = dash + 1;
    path->field_index = atoi(ptr);
    if (path->field_index <= 0) {
        return -1;
    }

    /* Parse component index (if present) */
    const char *dot = strchr(ptr, '.');
    if (dot) {
        ptr = dot + 1;
        path->component_index = atoi(ptr);

        /* Parse subcomponent index (if present) */
        dot = strchr(ptr, '.');
        if (dot) {
            ptr = dot + 1;
            path->subcomponent_index = atoi(ptr);
        }
    }

    return 0;
}

/**
 * Get field value using parsed path
 */
const char *hl7_message_get_field_by_path(const hl7_message_t *msg,
                                           const hl7_field_path_t *path) {
    if (!msg || !path) {
        return NULL;
    }

    /* Get segment */
    hl7_segment_t *seg = hl7_message_get_segment(msg, path->segment_id,
                                                   path->segment_index);
    if (!seg) {
        return NULL;
    }

    /* Get field */
    hl7_field_t *field = hl7_segment_get_field(seg, path->field_index);
    if (!field) {
        return NULL;
    }

    /* Handle repetitions */
    if (field->num_repetitions > 0 && path->repetition_index >= 0) {
        if ((size_t)path->repetition_index < field->num_repetitions) {
            field = &field->repetitions[path->repetition_index];
        } else {
            return NULL;
        }
    }

    /* If no component specified, return whole field */
    if (path->component_index < 0) {
        return hl7_field_get_value(field);
    }

    /* Get component */
    hl7_component_t *comp = hl7_field_get_component(field, path->component_index);
    if (!comp) {
        return NULL;
    }

    /* If no subcomponent specified, return whole component */
    if (path->subcomponent_index < 0) {
        return hl7_component_get_value(comp);
    }

    /* Get subcomponent */
    hl7_subcomponent_t *subcomp = hl7_component_get_subcomponent(comp,
                                                                   path->subcomponent_index);
    if (!subcomp) {
        return NULL;
    }

    return hl7_subcomponent_get_value(subcomp);
}

/**
 * Get field value from message using path notation
 */
const char *hl7_message_get_field(const hl7_message_t *msg, const char *path) {
    hl7_field_path_t parsed_path;

    if (hl7_parse_field_path(path, &parsed_path) != 0) {
        return NULL;
    }

    return hl7_message_get_field_by_path(msg, &parsed_path);
}

/**
 * Add segment to message
 */
int hl7_message_add_segment(hl7_message_t *msg, hl7_segment_t *segment) {
    if (!msg || !segment) {
        return -1;
    }

    /* Add to end of list */
    if (!msg->segments) {
        msg->segments = segment;
    } else {
        hl7_segment_t *last = msg->segments;
        while (last->next) {
            last = last->next;
        }
        last->next = segment;
    }

    segment->next = NULL;
    msg->num_segments++;

    /* Cache common segments */
    if (strcmp(segment->id, "MSH") == 0 && !msg->msh) {
        msg->msh = segment;
    } else if (strcmp(segment->id, "PID") == 0 && !msg->pid) {
        msg->pid = segment;
    }

    return 0;
}

/**
 * Create a new segment
 */
hl7_segment_t *hl7_segment_create(hl7_message_t *msg,
                                   const char *segment_id,
                                   size_t num_fields) {
    if (!msg || !segment_id) {
        return NULL;
    }

    memory_pool_t *pool = (memory_pool_t *)msg->memory_pool;

    hl7_segment_t *seg = memory_pool_calloc(pool, sizeof(hl7_segment_t));
    if (!seg) {
        return NULL;
    }

    strncpy(seg->id, segment_id, HL7_MAX_SEGMENT_ID_LEN);
    seg->id[HL7_MAX_SEGMENT_ID_LEN] = '\0';

    if (num_fields > 0) {
        seg->fields = memory_pool_calloc(pool, sizeof(hl7_field_t) * num_fields);
        if (!seg->fields) {
            return NULL;
        }
        seg->num_fields = num_fields;
    }

    return seg;
}

/**
 * Get field from segment
 */
hl7_field_t *hl7_segment_get_field(const hl7_segment_t *segment, int field_index) {
    if (!segment || field_index <= 0) {
        return NULL;
    }

    /* Convert to 0-based index */
    size_t idx = (size_t)(field_index - 1);

    if (idx >= segment->num_fields) {
        return NULL;
    }

    return &segment->fields[idx];
}

/**
 * Get field value as string
 */
const char *hl7_field_get_value(const hl7_field_t *field) {
    if (!field) {
        return NULL;
    }

    /* If field has direct value, return it */
    if (field->value) {
        return field->value;
    }

    /* If field has components, return the first component's value */
    if (field->num_components > 0 && field->components[0].value) {
        return field->components[0].value;
    }

    /* No value available */
    return NULL;
}

/**
 * Get component from field
 */
hl7_component_t *hl7_field_get_component(const hl7_field_t *field,
                                          int component_index) {
    if (!field || component_index < 0) {
        return NULL;
    }

    if ((size_t)component_index >= field->num_components) {
        return NULL;
    }

    return &field->components[component_index];
}

/**
 * Get component value as string
 */
const char *hl7_component_get_value(const hl7_component_t *component) {
    if (!component) {
        return NULL;
    }

    /* If component has direct value, return it */
    if (component->value) {
        return component->value;
    }

    /* If component has subcomponents, return the first one's value */
    if (component->num_subcomponents > 0) {
        return component->subcomponents[0].value;
    }

    /* No value available */
    return NULL;
}

/**
 * Get subcomponent from component
 */
hl7_subcomponent_t *hl7_component_get_subcomponent(const hl7_component_t *component,
                                                     int subcomponent_index) {
    if (!component || subcomponent_index < 0) {
        return NULL;
    }

    if ((size_t)subcomponent_index >= component->num_subcomponents) {
        return NULL;
    }

    return &component->subcomponents[subcomponent_index];
}

/**
 * Get subcomponent value
 */
const char *hl7_subcomponent_get_value(const hl7_subcomponent_t *subcomponent) {
    return subcomponent ? subcomponent->value : NULL;
}

/**
 * Get message type enum from string
 */
hl7_message_type_t hl7_get_message_type(const char *msg_type_str) {
    if (!msg_type_str) {
        return HL7_MSG_UNKNOWN;
    }

    if (strncmp(msg_type_str, "ADT", 3) == 0) return HL7_MSG_ADT;
    if (strncmp(msg_type_str, "ORU", 3) == 0) return HL7_MSG_ORU;
    if (strncmp(msg_type_str, "ORM", 3) == 0) return HL7_MSG_ORM;
    if (strncmp(msg_type_str, "DFT", 3) == 0) return HL7_MSG_DFT;
    if (strncmp(msg_type_str, "SIU", 3) == 0) return HL7_MSG_SIU;
    if (strncmp(msg_type_str, "MDM", 3) == 0) return HL7_MSG_MDM;
    if (strncmp(msg_type_str, "ACK", 3) == 0) return HL7_MSG_ACK;
    if (strncmp(msg_type_str, "QRY", 3) == 0) return HL7_MSG_QRY;
    if (strncmp(msg_type_str, "RAS", 3) == 0) return HL7_MSG_RAS;
    if (strncmp(msg_type_str, "BAR", 3) == 0) return HL7_MSG_BAR;

    return HL7_MSG_UNKNOWN;
}

/**
 * Get message type string from enum
 */
const char *hl7_message_type_to_string(hl7_message_type_t msg_type) {
    switch (msg_type) {
        case HL7_MSG_ADT: return "ADT";
        case HL7_MSG_ORU: return "ORU";
        case HL7_MSG_ORM: return "ORM";
        case HL7_MSG_DFT: return "DFT";
        case HL7_MSG_SIU: return "SIU";
        case HL7_MSG_MDM: return "MDM";
        case HL7_MSG_ACK: return "ACK";
        case HL7_MSG_QRY: return "QRY";
        case HL7_MSG_RAS: return "RAS";
        case HL7_MSG_BAR: return "BAR";
        default: return "UNKNOWN";
    }
}

/**
 * Get HL7 version enum from string
 */
hl7_version_t hl7_get_version(const char *version_str) {
    if (!version_str) {
        return HL7_VERSION_UNKNOWN;
    }

    if (strcmp(version_str, "2.1") == 0) return HL7_VERSION_2_1;
    if (strcmp(version_str, "2.2") == 0) return HL7_VERSION_2_2;
    if (strcmp(version_str, "2.3") == 0) return HL7_VERSION_2_3;
    if (strcmp(version_str, "2.3.1") == 0) return HL7_VERSION_2_3_1;
    if (strcmp(version_str, "2.4") == 0) return HL7_VERSION_2_4;
    if (strcmp(version_str, "2.5") == 0) return HL7_VERSION_2_5;
    if (strcmp(version_str, "2.5.1") == 0) return HL7_VERSION_2_5_1;
    if (strcmp(version_str, "2.6") == 0) return HL7_VERSION_2_6;
    if (strcmp(version_str, "2.7") == 0) return HL7_VERSION_2_7;
    if (strcmp(version_str, "2.8") == 0) return HL7_VERSION_2_8;

    return HL7_VERSION_UNKNOWN;
}

/**
 * Get version string from enum
 */
const char *hl7_version_to_string(hl7_version_t version) {
    switch (version) {
        case HL7_VERSION_2_1: return "2.1";
        case HL7_VERSION_2_2: return "2.2";
        case HL7_VERSION_2_3: return "2.3";
        case HL7_VERSION_2_3_1: return "2.3.1";
        case HL7_VERSION_2_4: return "2.4";
        case HL7_VERSION_2_5: return "2.5";
        case HL7_VERSION_2_5_1: return "2.5.1";
        case HL7_VERSION_2_6: return "2.6";
        case HL7_VERSION_2_7: return "2.7";
        case HL7_VERSION_2_8: return "2.8";
        default: return "unknown";
    }
}

/**
 * Print message structure for debugging
 */
void hl7_message_print(const hl7_message_t *msg, int verbose) {
    if (!msg) {
        printf("NULL message\n");
        return;
    }

    printf("=== HL7 Message ===\n");
    printf("ID: %lu\n", (unsigned long)msg->msg_id);
    printf("Type: %s (%s)\n", msg->msg_type_str,
           hl7_message_type_to_string(msg->msg_type));
    printf("Version: %s\n", hl7_version_to_string(msg->version));
    printf("Segments: %zu\n", msg->num_segments);

    if (verbose) {
        hl7_segment_t *seg = msg->segments;
        int seg_num = 0;
        while (seg) {
            printf("\n[%d] %s (%zu fields)\n", seg_num++, seg->id, seg->num_fields);
            if (seg->raw_data) {
                printf("    Raw: %.*s\n", (int)seg->raw_length, seg->raw_data);
            }
            seg = seg->next;
        }
    }

    printf("==================\n");
}

/**
 * Print message structure with PHI redaction (HIPAA-compliant)
 */
void hl7_message_print_redacted(const hl7_message_t *msg,
                                 int verbose,
                                 const void *config) {
    if (!msg) {
        printf("NULL message\n");
        return;
    }

    const phi_redaction_config_t *phi_config = (const phi_redaction_config_t *)config;
    phi_redaction_config_t default_config;

    /* Use default config if none provided */
    if (!phi_config) {
        default_config = phi_redaction_get_default_config();
        phi_config = &default_config;
    }

    printf("=== HL7 Message (PHI REDACTED) ===\n");
    printf("ID: %lu\n", (unsigned long)msg->msg_id);
    printf("Type: %s (%s)\n", msg->msg_type_str,
           hl7_message_type_to_string(msg->msg_type));
    printf("Version: %s\n", hl7_version_to_string(msg->version));
    printf("Segments: %zu\n", msg->num_segments);

    if (verbose) {
        hl7_segment_t *seg = msg->segments;
        int seg_num = 0;

        while (seg) {
            printf("\n[%d] %s (%zu fields)\n", seg_num++, seg->id, seg->num_fields);

            /* Print fields with redaction */
            for (size_t i = 0; i < seg->num_fields && i < HL7_MAX_FIELDS; i++) {
                hl7_field_t *field = &seg->fields[i];
                if (!field->value) continue;

                /* Create field identifier (e.g., "PID-3") */
                char field_id[16];
                snprintf(field_id, sizeof(field_id), "%s-%zu", seg->id, i + 1);

                /* Redact field value if necessary */
                char *display_value = phi_redact_field(field_id, field->value, phi_config);

                if (display_value) {
                    printf("    [%zu] %s\n", i + 1, display_value);
                    free(display_value);
                }
            }

            seg = seg->next;
        }
    }

    printf("===================================\n");
}
