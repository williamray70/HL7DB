/**
 * hl7_validator.h - HL7 Message Validation
 *
 * Validates HL7 messages against v2.x specifications.
 */

#ifndef HL7DB_HL7_VALIDATOR_H
#define HL7DB_HL7_VALIDATOR_H

#include "hl7/hl7_types.h"

/**
 * Validation error codes
 */
typedef enum {
    HL7_VALID = 0,
    HL7_ERR_NO_MSH = -1,
    HL7_ERR_MSH_NOT_FIRST = -2,
    HL7_ERR_NO_SEGMENTS = -3,
    HL7_ERR_PARSE_ERRORS = -4,
    HL7_ERR_INVALID_STRUCTURE = -5,
} hl7_validation_error_t;

/**
 * Validate message structure and content
 *
 * @param msg Message to validate
 * @return 0 if valid, error code otherwise
 */
int hl7_validator_validate(const hl7_message_t *msg);

/**
 * Get validation error description
 *
 * @param error Error code
 * @return Error description string
 */
const char *hl7_validator_get_error_string(int error);

#endif /* HL7DB_HL7_VALIDATOR_H */
