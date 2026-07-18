/**
 * hl7_validator.c - HL7 Message Validation Implementation
 */

#include "hl7/hl7_validator.h"
#include "hl7/hl7_parser.h"
#include "util/logger.h"

/**
 * Validate message structure and content
 */
int hl7_validator_validate(const hl7_message_t *msg) {
    /* Reuse the validation logic from parser */
    return hl7_validate_message(msg);
}

/**
 * Get validation error description
 */
const char *hl7_validator_get_error_string(int error) {
    switch (error) {
        case HL7_VALID:
            return "Message is valid";
        case HL7_ERR_NO_MSH:
            return "Message missing MSH segment";
        case HL7_ERR_MSH_NOT_FIRST:
            return "MSH must be first segment";
        case HL7_ERR_NO_SEGMENTS:
            return "Message has no segments";
        case HL7_ERR_PARSE_ERRORS:
            return "Message has parse errors";
        case HL7_ERR_INVALID_STRUCTURE:
            return "Invalid message structure";
        default:
            return "Unknown error";
    }
}
