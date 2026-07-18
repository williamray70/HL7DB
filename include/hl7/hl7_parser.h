/**
 * hl7_parser.h - HL7 v2.x Message Parser
 *
 * Parses pipe-delimited HL7 v2.x messages into hierarchical structures.
 */

#ifndef HL7DB_HL7_PARSER_H
#define HL7DB_HL7_PARSER_H

#include "hl7/hl7_types.h"
#include "hl7/hl7_message.h"

/**
 * Parse HL7 message from string
 *
 * @param data Raw HL7 message data
 * @param length Length of data
 * @param options Parse options (NULL = use defaults)
 * @return Parsed message, or NULL on failure
 *
 * Note: Caller must call hl7_message_destroy() on returned message
 */
hl7_message_t *hl7_parse_message(const char *data,
                                  size_t length,
                                  const hl7_parse_options_t *options);

/**
 * Parse HL7 message from null-terminated string
 *
 * @param data Raw HL7 message data (null-terminated)
 * @param options Parse options (NULL = use defaults)
 * @return Parsed message, or NULL on failure
 */
hl7_message_t *hl7_parse(const char *data, const hl7_parse_options_t *options);

/**
 * Get default parse options
 *
 * @return Default parse options structure
 */
hl7_parse_options_t hl7_get_default_parse_options(void);

/**
 * Parse MSH segment and extract encoding characters
 *
 * @param msh_data MSH segment data
 * @param length Length of MSH data
 * @param encoding Pointer to encoding structure to fill
 * @return 0 on success, -1 on failure
 */
int hl7_parse_msh_encoding(const char *msh_data,
                            size_t length,
                            hl7_encoding_chars_t *encoding);

/**
 * Validate message structure
 *
 * @param msg Message to validate
 * @return 0 if valid, error code otherwise
 */
int hl7_validate_message(const hl7_message_t *msg);

#endif /* HL7DB_HL7_PARSER_H */
