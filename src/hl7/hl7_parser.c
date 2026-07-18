/**
 * hl7_parser.c - HL7 v2.x Message Parser Implementation
 */

#include "hl7/hl7_parser.h"
#include "hl7/hl7_message.h"
#include "hl7/hl7_types.h"
#include "util/memory.h"
#include "util/logger.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Get default parse options
 */
hl7_parse_options_t hl7_get_default_parse_options(void) {
    hl7_parse_options_t opts = {
        .strict_mode = 0,
        .allow_non_standard_delimiters = 1,
        .validate_structure = 1,
        .trim_whitespace = 1,
        .decode_escape_sequences = 0  /* Future feature */
    };
    return opts;
}

/**
 * Parse MSH segment and extract encoding characters
 */
int hl7_parse_msh_encoding(const char *msh_data,
                            size_t length,
                            hl7_encoding_chars_t *encoding) {
    if (!msh_data || length < 8 || !encoding) {
        return -1;
    }

    /* MSH format: MSH|^~\&|... */
    if (strncmp(msh_data, "MSH", 3) != 0) {
        return -1;
    }

    /* Field separator (position 3) */
    encoding->field_separator = msh_data[3];

    /* Encoding characters from MSH-2 (positions 4-7) */
    if (length >= 8) {
        encoding->component_separator = msh_data[4];
        encoding->repetition_separator = msh_data[5];
        encoding->escape_character = msh_data[6];
        encoding->subcomponent_separator = msh_data[7];
    } else {
        /* Use defaults if truncated */
        encoding->component_separator = HL7_COMPONENT_DELIMITER;
        encoding->repetition_separator = HL7_REPETITION_DELIMITER;
        encoding->escape_character = HL7_ESCAPE_CHAR;
        encoding->subcomponent_separator = HL7_SUBCOMPONENT_DELIMITER;
    }

    return 0;
}

/**
 * Split string by delimiter
 */
static int split_string(const char *str, size_t length, char delimiter,
                        char ***parts, size_t *count, memory_pool_t *pool) {
    if (!str || !parts || !count || !pool) {
        return -1;
    }

    /* Count delimiters */
    size_t num_parts = 1;
    for (size_t i = 0; i < length; i++) {
        if (str[i] == delimiter) {
            num_parts++;
        }
    }

    /* Allocate parts array */
    char **result = memory_pool_alloc_array(pool, num_parts + 1, sizeof(char *));
    if (!result) {
        return -1;
    }

    /* Split string */
    size_t part_idx = 0;
    const char *start = str;
    const char *end = str;

    for (size_t i = 0; i <= length; i++) {
        if (i == length || str[i] == delimiter) {
            size_t part_len = end - start;

            /* Create part string */
            char *part = memory_pool_alloc(pool, part_len + 1);
            if (!part) {
                return -1;
            }

            if (part_len > 0) {
                memcpy(part, start, part_len);
            }
            part[part_len] = '\0';

            result[part_idx++] = part;

            if (i < length) {
                start = end = str + i + 1;
            }
        } else {
            end++;
        }
    }

    *parts = result;
    *count = num_parts;
    return 0;
}

/**
 * Parse subcomponents
 */
static int parse_subcomponents(const char *data, size_t length,
                                hl7_component_t *component,
                                const hl7_encoding_chars_t *encoding,
                                memory_pool_t *pool) {
    char **parts = NULL;
    size_t num_parts = 0;

    if (split_string(data, length, encoding->subcomponent_separator,
                     &parts, &num_parts, pool) != 0) {
        return -1;
    }

    if (num_parts == 1) {
        /* No subcomponents - just store value */
        component->value = parts[0];
        component->length = strlen(parts[0]);
        component->subcomponents = NULL;
        component->num_subcomponents = 0;
    } else {
        /* Multiple subcomponents */
        component->subcomponents = memory_pool_alloc_array(pool, num_parts,
                                                            sizeof(hl7_subcomponent_t));
        if (!component->subcomponents) {
            return -1;
        }

        for (size_t i = 0; i < num_parts; i++) {
            component->subcomponents[i].value = parts[i];
            component->subcomponents[i].length = strlen(parts[i]);
        }

        component->num_subcomponents = num_parts;
        component->value = NULL;
    }

    return 0;
}

/**
 * Parse components
 */
static int parse_components(const char *data, size_t length,
                             hl7_field_t *field,
                             const hl7_encoding_chars_t *encoding,
                             memory_pool_t *pool) {
    char **parts = NULL;
    size_t num_parts = 0;

    if (split_string(data, length, encoding->component_separator,
                     &parts, &num_parts, pool) != 0) {
        return -1;
    }

    if (num_parts == 1) {
        /* No components - check for subcomponents */
        hl7_component_t *comp = memory_pool_calloc(pool, sizeof(hl7_component_t));
        if (!comp) {
            return -1;
        }

        if (parse_subcomponents(parts[0], strlen(parts[0]), comp, encoding, pool) != 0) {
            return -1;
        }

        field->components = comp;
        field->num_components = 1;

        /* If single component with single subcomponent, set field value */
        if (comp->num_subcomponents == 0 && comp->value) {
            field->value = comp->value;
            field->length = comp->length;
        } else {
            field->value = NULL;
        }
    } else {
        /* Multiple components */
        field->components = memory_pool_alloc_array(pool, num_parts,
                                                     sizeof(hl7_component_t));
        if (!field->components) {
            return -1;
        }

        for (size_t i = 0; i < num_parts; i++) {
            if (parse_subcomponents(parts[i], strlen(parts[i]),
                                     &field->components[i], encoding, pool) != 0) {
                return -1;
            }
        }

        field->num_components = num_parts;
        field->value = NULL;
    }

    return 0;
}

/**
 * Parse field (handles repetitions)
 */
static int parse_field(const char *data, size_t length,
                        hl7_field_t *field,
                        const hl7_encoding_chars_t *encoding,
                        memory_pool_t *pool) {
    char **parts = NULL;
    size_t num_parts = 0;

    /* Check for repetitions */
    if (split_string(data, length, encoding->repetition_separator,
                     &parts, &num_parts, pool) != 0) {
        return -1;
    }

    if (num_parts == 1) {
        /* No repetitions */
        field->repetitions = NULL;
        field->num_repetitions = 0;

        return parse_components(parts[0], strlen(parts[0]), field, encoding, pool);
    } else {
        /* Multiple repetitions */
        field->repetitions = memory_pool_alloc_array(pool, num_parts,
                                                      sizeof(hl7_field_t));
        if (!field->repetitions) {
            return -1;
        }

        for (size_t i = 0; i < num_parts; i++) {
            if (parse_components(parts[i], strlen(parts[i]),
                                  &field->repetitions[i], encoding, pool) != 0) {
                return -1;
            }
        }

        field->num_repetitions = num_parts;

        /* First repetition's value becomes the field value */
        field->value = field->repetitions[0].value;
        field->components = field->repetitions[0].components;
        field->num_components = field->repetitions[0].num_components;
    }

    return 0;
}

/**
 * Parse segment
 */
static hl7_segment_t *parse_segment(const char *segment_data, size_t length,
                                     const hl7_encoding_chars_t *encoding,
                                     hl7_message_t *msg) {
    memory_pool_t *pool = (memory_pool_t *)msg->memory_pool;

    /* Extract segment ID (first 3 characters) */
    if (length < 3) {
        LOG_ERROR("Segment too short: %zu bytes", length);
        return NULL;
    }

    char segment_id[4];
    memcpy(segment_id, segment_data, 3);
    segment_id[3] = '\0';

    /* Split into fields */
    char **field_parts = NULL;
    size_t num_fields = 0;

    if (split_string(segment_data, length, encoding->field_separator,
                     &field_parts, &num_fields, pool) != 0) {
        LOG_ERROR("Failed to split segment into fields");
        return NULL;
    }

    /* Create segment */
    hl7_segment_t *seg = hl7_segment_create(msg, segment_id, num_fields - 1);
    if (!seg) {
        LOG_ERROR("Failed to create segment");
        return NULL;
    }

    /* Store raw data */
    seg->raw_data = memory_pool_alloc(pool, length + 1);
    if (seg->raw_data) {
        memcpy(seg->raw_data, segment_data, length);
        seg->raw_data[length] = '\0';
        seg->raw_length = length;
    }

    /* Parse fields (skip field 0 which is the segment ID) */
    for (size_t i = 1; i < num_fields; i++) {
        if (parse_field(field_parts[i], strlen(field_parts[i]),
                         &seg->fields[i - 1], encoding, pool) != 0) {
            LOG_WARN("Failed to parse field %zu in segment %s", i, segment_id);
        }
    }

    return seg;
}

/**
 * Parse HL7 message
 */
hl7_message_t *hl7_parse_message(const char *data,
                                  size_t length,
                                  const hl7_parse_options_t *options) {
    if (!data || length == 0) {
        LOG_ERROR("Invalid input data");
        return NULL;
    }

    /* Use default options if not provided */
    hl7_parse_options_t opts = options ? *options : hl7_get_default_parse_options();

    /* Create message */
    hl7_message_t *msg = hl7_message_create();
    if (!msg) {
        LOG_ERROR("Failed to create message");
        return NULL;
    }

    memory_pool_t *pool = (memory_pool_t *)msg->memory_pool;

    /* Store raw message */
    msg->raw_message = memory_pool_alloc(pool, length + 1);
    if (!msg->raw_message) {
        hl7_message_destroy(msg);
        return NULL;
    }
    memcpy(msg->raw_message, data, length);
    msg->raw_message[length] = '\0';
    msg->raw_length = length;

    /* Parse MSH segment first to get encoding characters */
    if (length < 8 || strncmp(data, "MSH", 3) != 0) {
        LOG_ERROR("Message must start with MSH segment");
        msg->error_message = "Message must start with MSH segment";
        msg->parse_errors++;
        return msg;  /* Return partial message for inspection */
    }

    if (hl7_parse_msh_encoding(data, length, &msg->encoding) != 0) {
        LOG_ERROR("Failed to parse MSH encoding characters");
        msg->error_message = "Failed to parse MSH encoding";
        msg->parse_errors++;
        return msg;
    }

    LOG_DEBUG("Encoding chars: field='%c' component='%c' repetition='%c' escape='%c' subcomponent='%c'",
              msg->encoding.field_separator, msg->encoding.component_separator,
              msg->encoding.repetition_separator, msg->encoding.escape_character,
              msg->encoding.subcomponent_separator);

    /* Split message into segments */
    char **segments = NULL;
    size_t num_segments = 0;

    if (split_string(data, length, HL7_SEGMENT_DELIMITER,
                     &segments, &num_segments, pool) != 0) {
        LOG_ERROR("Failed to split message into segments");
        hl7_message_destroy(msg);
        return NULL;
    }

    LOG_INFO("Parsing %zu segments", num_segments);

    /* Parse each segment */
    for (size_t i = 0; i < num_segments; i++) {
        size_t seg_len = strlen(segments[i]);

        /* Skip empty segments */
        if (seg_len == 0 || (opts.trim_whitespace && isspace(segments[i][0]))) {
            continue;
        }

        hl7_segment_t *seg = parse_segment(segments[i], seg_len, &msg->encoding, msg);
        if (!seg) {
            LOG_WARN("Failed to parse segment %zu", i);
            msg->parse_errors++;
            continue;
        }

        if (hl7_message_add_segment(msg, seg) != 0) {
            LOG_ERROR("Failed to add segment to message");
            msg->parse_errors++;
        }
    }

    /* Extract message type from MSH-9 */
    if (msg->msh) {
        const char *msg_type = hl7_message_get_field(msg, "MSH-9");
        if (msg_type) {
            strncpy(msg->msg_type_str, msg_type, HL7_MAX_MESSAGE_TYPE_LEN - 1);
            msg->msg_type_str[HL7_MAX_MESSAGE_TYPE_LEN - 1] = '\0';
            msg->msg_type = hl7_get_message_type(msg_type);
            LOG_DEBUG("Message type: %s (%d)", msg->msg_type_str, msg->msg_type);
        }

        /* Extract version from MSH-12 */
        const char *version = hl7_message_get_field(msg, "MSH-12");
        if (version) {
            msg->version = hl7_get_version(version);
            LOG_DEBUG("HL7 version: %s", version);
        }
    }

    LOG_INFO("Parsed message: %zu segments, %d errors",
             msg->num_segments, msg->parse_errors);

    return msg;
}

/**
 * Parse HL7 message from null-terminated string
 */
hl7_message_t *hl7_parse(const char *data, const hl7_parse_options_t *options) {
    if (!data) {
        return NULL;
    }
    return hl7_parse_message(data, strlen(data), options);
}

/**
 * Validate message structure
 */
int hl7_validate_message(const hl7_message_t *msg) {
    if (!msg) {
        return -1;
    }

    /* Must have MSH segment */
    if (!msg->msh) {
        LOG_ERROR("Message missing MSH segment");
        return -2;
    }

    /* MSH must be first segment */
    if (msg->segments != msg->msh) {
        LOG_ERROR("MSH must be first segment");
        return -3;
    }

    /* Must have at least one segment */
    if (msg->num_segments == 0) {
        LOG_ERROR("Message has no segments");
        return -4;
    }

    /* Check for parse errors */
    if (msg->parse_errors > 0) {
        LOG_WARN("Message has %d parse errors", msg->parse_errors);
        return -5;
    }

    return 0;
}
