/**
 * phi_redaction.c - HIPAA PHI Redaction Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include "util/phi_redaction.h"
#include "util/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Simple hash function for PHI_REDACT_HASH mode */
static unsigned int simple_hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/**
 * Get default PHI redaction configuration
 */
phi_redaction_config_t phi_redaction_get_default_config(void) {
    phi_redaction_config_t config = {
        .mode = PHI_REDACT_FULL,
        .redact_names = true,
        .redact_ids = true,
        .redact_addresses = true,
        .redact_dates = true,
        .redact_phone = true,
        .redact_email = true,
        .redact_accounts = true,
        .allow_msg_type = true,   /* MSH-9 is not PHI */
        .allow_facility = true    /* Facility codes typically not PHI */
    };
    return config;
}

/**
 * Get debug configuration (partial redaction)
 */
phi_redaction_config_t phi_redaction_get_debug_config(void) {
    phi_redaction_config_t config = {
        .mode = PHI_REDACT_PARTIAL,
        .redact_names = true,
        .redact_ids = true,
        .redact_addresses = true,
        .redact_dates = false,    /* Allow dates for debugging */
        .redact_phone = true,
        .redact_email = true,
        .redact_accounts = true,
        .allow_msg_type = true,
        .allow_facility = true
    };
    return config;
}

/**
 * Get no-redaction configuration
 */
phi_redaction_config_t phi_redaction_get_none_config(void) {
    phi_redaction_config_t config = {
        .mode = PHI_REDACT_NONE,
        .redact_names = false,
        .redact_ids = false,
        .redact_addresses = false,
        .redact_dates = false,
        .redact_phone = false,
        .redact_email = false,
        .redact_accounts = false,
        .allow_msg_type = true,
        .allow_facility = true
    };
    return config;
}

/**
 * Check if field contains PHI based on HL7 standards
 */
bool phi_is_phi_field(const char *field_id) {
    if (!field_id) return false;

    /* HIPAA defines 18 identifiers - we focus on the most common in HL7 */

    /* Patient Identifying Information (PID segment) */
    if (strncmp(field_id, "PID-", 4) == 0) {
        const char *field_num = field_id + 4;

        /* PID-3: Patient ID (MRN, SSN, etc.) */
        if (strcmp(field_num, "3") == 0) return true;

        /* PID-5: Patient Name */
        if (strcmp(field_num, "5") == 0) return true;

        /* PID-7: Date of Birth */
        if (strcmp(field_num, "7") == 0) return true;

        /* PID-11: Patient Address */
        if (strcmp(field_num, "11") == 0) return true;

        /* PID-13: Phone Number */
        if (strcmp(field_num, "13") == 0) return true;

        /* PID-14: Business Phone */
        if (strcmp(field_num, "14") == 0) return true;

        /* PID-18: Account Number */
        if (strcmp(field_num, "18") == 0) return true;

        /* PID-19: SSN */
        if (strcmp(field_num, "19") == 0) return true;

        /* PID-20: Driver's License */
        if (strcmp(field_num, "20") == 0) return true;
    }

    /* Next of Kin (NK1 segment) */
    if (strncmp(field_id, "NK1-", 4) == 0) {
        const char *field_num = field_id + 4;

        /* NK1-2: Name */
        if (strcmp(field_num, "2") == 0) return true;

        /* NK1-4: Address */
        if (strcmp(field_num, "4") == 0) return true;

        /* NK1-5: Phone */
        if (strcmp(field_num, "5") == 0) return true;
    }

    /* Insurance Information (IN1 segment) */
    if (strncmp(field_id, "IN1-", 4) == 0) {
        const char *field_num = field_id + 4;

        /* IN1-2: Insurance Plan ID */
        if (strcmp(field_num, "2") == 0) return true;

        /* IN1-36: Policy Number */
        if (strcmp(field_num, "36") == 0) return true;
    }

    /* Provider Information (can be PHI in some contexts) */
    if (strncmp(field_id, "ORC-", 4) == 0 ||
        strncmp(field_id, "OBR-", 4) == 0) {
        /* Provider names and IDs can be PHI under HIPAA */
        /* For now, we're conservative and don't redact unless explicitly needed */
    }

    /* Dates and times (can be PHI if specific to patient events) */
    if (strncmp(field_id, "PV1-", 4) == 0) {
        const char *field_num = field_id + 4;

        /* PV1-44: Admit Date/Time */
        if (strcmp(field_num, "44") == 0) return true;

        /* PV1-45: Discharge Date/Time */
        if (strcmp(field_num, "45") == 0) return true;
    }

    /* Medical Record Numbers in other contexts */
    if (strstr(field_id, "MRN") || strstr(field_id, "SSN")) {
        return true;
    }

    return false;
}

/**
 * Redact a value using specified mode
 */
char *phi_redact_value(const char *value, phi_redaction_mode_t mode) {
    if (!value) return NULL;

    size_t len = strlen(value);
    char *redacted = NULL;

    switch (mode) {
        case PHI_REDACT_NONE:
            /* Return copy of original */
            redacted = strdup(value);
            break;

        case PHI_REDACT_PARTIAL: {
            /* Show first and last character(s), redact middle */
            if (len <= 2) {
                /* Too short, redact completely */
                redacted = strdup("[REDACTED]");
            } else if (len <= 4) {
                /* Show first char only: "John" -> "J***" */
                redacted = malloc(len + 4);
                if (redacted) {
                    snprintf(redacted, len + 4, "%c***", value[0]);
                }
            } else {
                /* Show first and last: "John Doe" -> "J**** D**" */
                size_t redact_len = len + 10;
                redacted = malloc(redact_len);
                if (redacted) {
                    /* Find last non-space character */
                    size_t last_pos = len - 1;
                    while (last_pos > 0 && isspace(value[last_pos])) {
                        last_pos--;
                    }

                    /* Simple partial redaction */
                    snprintf(redacted, redact_len, "%c***%c***",
                             value[0], value[last_pos]);
                }
            }
            break;
        }

        case PHI_REDACT_FULL:
            /* Complete redaction */
            redacted = strdup("[REDACTED]");
            break;

        case PHI_REDACT_HASH: {
            /* Show hash for debugging correlation */
            unsigned int hash = simple_hash(value);
            redacted = malloc(32);
            if (redacted) {
                snprintf(redacted, 32, "[PHI:%08x]", hash);
            }
            break;
        }

        default:
            redacted = strdup("[REDACTED]");
            break;
    }

    return redacted;
}

/**
 * Determine if field should be redacted based on config
 */
static bool should_redact_field(const char *field_id,
                                 const phi_redaction_config_t *config) {
    if (!config || config->mode == PHI_REDACT_NONE) {
        return false;
    }

    /* Allow message type if configured */
    if (config->allow_msg_type && strcmp(field_id, "MSH-9") == 0) {
        return false;
    }

    /* Allow facility codes if configured */
    if (config->allow_facility &&
        (strcmp(field_id, "MSH-3") == 0 || strcmp(field_id, "MSH-4") == 0 ||
         strcmp(field_id, "MSH-5") == 0 || strcmp(field_id, "MSH-6") == 0)) {
        return false;
    }

    /* Check if this is a known PHI field */
    if (!phi_is_phi_field(field_id)) {
        return false;
    }

    /* Check specific field type against config */
    if (strstr(field_id, "PID-5") || strstr(field_id, "NK1-2")) {
        return config->redact_names;
    }

    if (strstr(field_id, "PID-3") || strstr(field_id, "PID-19")) {
        return config->redact_ids;
    }

    if (strstr(field_id, "PID-11") || strstr(field_id, "NK1-4")) {
        return config->redact_addresses;
    }

    if (strstr(field_id, "PID-7") || strstr(field_id, "PV1-44") ||
        strstr(field_id, "PV1-45")) {
        return config->redact_dates;
    }

    if (strstr(field_id, "PID-13") || strstr(field_id, "PID-14") ||
        strstr(field_id, "NK1-5")) {
        return config->redact_phone;
    }

    if (strstr(field_id, "PID-18") || strstr(field_id, "IN1-")) {
        return config->redact_accounts;
    }

    /* Default to redacting if it's a PHI field */
    return true;
}

/**
 * Redact a single field value
 */
char *phi_redact_field(const char *field_id,
                       const char *value,
                       const phi_redaction_config_t *config) {
    if (!value) return NULL;

    phi_redaction_config_t default_config;
    if (!config) {
        /* No config = use default (full redaction) */
        default_config = phi_redaction_get_default_config();
        config = &default_config;
    }

    if (!should_redact_field(field_id, config)) {
        return strdup(value);
    }

    return phi_redact_value(value, config->mode);
}

/**
 * Redact an entire HL7 message
 */
char *phi_redact_message(const char *raw_message,
                         const phi_redaction_config_t *config) {
    if (!raw_message) return NULL;

    /* If no redaction, return copy */
    if (config && config->mode == PHI_REDACT_NONE) {
        return strdup(raw_message);
    }

    /* Use default config if none provided */
    phi_redaction_config_t default_config;
    if (!config) {
        default_config = phi_redaction_get_default_config();
        config = &default_config;
    }

    /* Allocate buffer for redacted message (same size + extra for redaction markers) */
    size_t msg_len = strlen(raw_message);
    size_t buffer_size = msg_len * 2;  /* Extra space for redaction markers */
    char *redacted = malloc(buffer_size);
    if (!redacted) return NULL;

    char *out = redacted;
    const char *in = raw_message;
    const char *segment_start = in;
    char current_segment[4] = {0};
    int field_num = 0;
    bool in_field = false;
    char field_buffer[1024];
    size_t field_pos = 0;

    /* Parse and redact field by field */
    while (*in) {
        char c = *in;

        /* Segment boundary (carriage return) */
        if (c == '\r') {
            /* Flush current field if any */
            if (in_field && field_pos > 0) {
                field_buffer[field_pos] = '\0';

                char field_id[16];
                snprintf(field_id, sizeof(field_id), "%s-%d", current_segment, field_num);

                if (should_redact_field(field_id, config)) {
                    char *redacted_field = phi_redact_value(field_buffer, config->mode);
                    if (redacted_field) {
                        size_t remaining = buffer_size - (out - redacted);
                        snprintf(out, remaining, "%s", redacted_field);
                        out += strlen(redacted_field);
                        free(redacted_field);
                    }
                } else {
                    size_t remaining = buffer_size - (out - redacted);
                    snprintf(out, remaining, "%s", field_buffer);
                    out += strlen(field_buffer);
                }
            }

            *out++ = c;
            in++;
            segment_start = in;
            field_num = 0;
            in_field = false;
            field_pos = 0;
            memset(current_segment, 0, sizeof(current_segment));
            continue;
        }

        /* Field separator */
        if (c == '|') {
            /* Extract segment name if this is first field */
            if (field_num == 0) {
                size_t seg_len = in - segment_start;
                if (seg_len >= 3) {
                    strncpy(current_segment, segment_start, 3);
                    current_segment[3] = '\0';
                }
            }

            /* Flush current field */
            if (in_field && field_pos > 0) {
                field_buffer[field_pos] = '\0';

                char field_id[16];
                snprintf(field_id, sizeof(field_id), "%s-%d", current_segment, field_num);

                if (should_redact_field(field_id, config)) {
                    char *redacted_field = phi_redact_value(field_buffer, config->mode);
                    if (redacted_field) {
                        size_t remaining = buffer_size - (out - redacted);
                        snprintf(out, remaining, "%s", redacted_field);
                        out += strlen(redacted_field);
                        free(redacted_field);
                    }
                } else {
                    size_t remaining = buffer_size - (out - redacted);
                    snprintf(out, remaining, "%s", field_buffer);
                    out += strlen(field_buffer);
                }
            }

            *out++ = c;
            in++;
            field_num++;
            in_field = true;
            field_pos = 0;
            continue;
        }

        /* Regular character - add to field buffer */
        if (in_field && field_pos < sizeof(field_buffer) - 1) {
            field_buffer[field_pos++] = c;
        } else if (!in_field) {
            /* Part of segment name or before first field */
            *out++ = c;
        }

        in++;
    }

    *out = '\0';
    return redacted;
}
