/**
 * phi_redaction.h - HIPAA PHI Redaction Utility
 *
 * Redacts Protected Health Information (PHI) from HL7 messages before
 * logging, displaying, or storing in non-secure contexts.
 *
 * HIPAA Security Rule (45 CFR 164.312) requires:
 * - Access controls to limit PHI exposure
 * - Audit controls that don't log PHI
 * - Transmission security that protects PHI
 *
 * This utility helps ensure PHI doesn't leak into logs, console output,
 * or error messages.
 */

#ifndef HL7DB_PHI_REDACTION_H
#define HL7DB_PHI_REDACTION_H

#include <stddef.h>
#include <stdbool.h>

/**
 * PHI redaction mode
 */
typedef enum {
    PHI_REDACT_NONE = 0,        /* No redaction (for secure contexts only) */
    PHI_REDACT_PARTIAL,         /* Show first/last chars: John Doe -> J*** D** */
    PHI_REDACT_FULL,            /* Complete redaction: John Doe -> [REDACTED] */
    PHI_REDACT_HASH             /* Show hash: John Doe -> [PHI:a3f2b1c4] */
} phi_redaction_mode_t;

/**
 * PHI redaction configuration
 */
typedef struct {
    phi_redaction_mode_t mode;  /* Redaction mode */
    bool redact_names;          /* Redact patient/provider names */
    bool redact_ids;            /* Redact patient IDs (MRN, SSN) */
    bool redact_addresses;      /* Redact street addresses */
    bool redact_dates;          /* Redact dates (birth, admission, etc.) */
    bool redact_phone;          /* Redact phone numbers */
    bool redact_email;          /* Redact email addresses */
    bool redact_accounts;       /* Redact account numbers */
    bool allow_msg_type;        /* Allow message type (MSH-9) - not PHI */
    bool allow_facility;        /* Allow facility codes - often not PHI */
} phi_redaction_config_t;

/**
 * Get default PHI redaction configuration
 *
 * Default: FULL redaction of all PHI types, allows msg type and facility
 *
 * Returns:
 *   Default configuration for HIPAA compliance
 */
phi_redaction_config_t phi_redaction_get_default_config(void);

/**
 * Get permissive redaction configuration (for debugging in secure environments)
 *
 * Uses PARTIAL redaction to help with debugging while still protecting PHI.
 * Only use in secure, access-controlled environments.
 *
 * Returns:
 *   Permissive configuration with partial redaction
 */
phi_redaction_config_t phi_redaction_get_debug_config(void);

/**
 * Get no-redaction configuration (for internal processing only)
 *
 * WARNING: Only use for internal processing where PHI access is authorized.
 * Never use for logs, console output, or external interfaces.
 *
 * Returns:
 *   Configuration with all redaction disabled
 */
phi_redaction_config_t phi_redaction_get_none_config(void);

/**
 * Redact a single field value based on field identifier
 *
 * Automatically detects PHI based on HL7 field semantics:
 * - PID-3: Patient ID (MRN)
 * - PID-5: Patient Name
 * - PID-11: Patient Address
 * - PID-13: Phone Number
 * - PID-7: Date of Birth
 * - etc.
 *
 * Args:
 *   field_id: HL7 field identifier (e.g., "PID-3", "PID-5")
 *   value: Original field value
 *   config: Redaction configuration
 *
 * Returns:
 *   Redacted string (dynamically allocated, caller must free)
 *   Returns NULL if value is NULL or on allocation failure
 */
char *phi_redact_field(const char *field_id,
                       const char *value,
                       const phi_redaction_config_t *config);

/**
 * Redact an entire HL7 message for safe display
 *
 * Creates a redacted copy of the raw HL7 message string with PHI
 * fields replaced according to configuration.
 *
 * Args:
 *   raw_message: Raw HL7 message string
 *   config: Redaction configuration
 *
 * Returns:
 *   Redacted message string (dynamically allocated, caller must free)
 *   Returns NULL if raw_message is NULL or on allocation failure
 */
char *phi_redact_message(const char *raw_message,
                         const phi_redaction_config_t *config);

/**
 * Check if a field contains PHI based on HL7 standards
 *
 * Args:
 *   field_id: HL7 field identifier (e.g., "PID-3")
 *
 * Returns:
 *   true if field typically contains PHI, false otherwise
 */
bool phi_is_phi_field(const char *field_id);

/**
 * Redact a string value using specified mode
 *
 * Low-level function for applying redaction to any string.
 *
 * Args:
 *   value: Original value
 *   mode: Redaction mode to apply
 *
 * Returns:
 *   Redacted string (dynamically allocated, caller must free)
 *   Returns NULL if value is NULL or on allocation failure
 */
char *phi_redact_value(const char *value, phi_redaction_mode_t mode);

#endif /* HL7DB_PHI_REDACTION_H */
