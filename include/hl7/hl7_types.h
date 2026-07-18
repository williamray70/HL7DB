/**
 * hl7_types.h - HL7 v2.x Data Type Definitions
 *
 * Hierarchical representation of HL7 messages:
 * Message -> Segments -> Fields -> Components -> Subcomponents
 */

#ifndef HL7DB_HL7_TYPES_H
#define HL7DB_HL7_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* HL7 Standard Delimiters */
#define HL7_SEGMENT_DELIMITER    '\r'    /* Carriage return */
#define HL7_FIELD_DELIMITER      '|'     /* Pipe */
#define HL7_COMPONENT_DELIMITER  '^'     /* Caret */
#define HL7_REPETITION_DELIMITER '~'     /* Tilde */
#define HL7_ESCAPE_CHAR          '\\'    /* Backslash */
#define HL7_SUBCOMPONENT_DELIMITER '&'   /* Ampersand */

/* MSH segment must start with this */
#define HL7_MSH_PREFIX "MSH|^~\\&|"

/* Maximum lengths */
#define HL7_MAX_SEGMENT_ID_LEN   3
#define HL7_MAX_MESSAGE_TYPE_LEN 16
#define HL7_MAX_FIELDS           256
#define HL7_MAX_COMPONENTS       32
#define HL7_MAX_SUBCOMPONENTS    8

/* Forward declarations */
typedef struct hl7_subcomponent hl7_subcomponent_t;
typedef struct hl7_component hl7_component_t;
typedef struct hl7_field hl7_field_t;
typedef struct hl7_segment hl7_segment_t;
typedef struct hl7_message hl7_message_t;

/**
 * HL7 Subcomponent (lowest level)
 * Example: In "PID|1||12345^JONES^MICHAEL^A", "A" is a subcomponent
 */
struct hl7_subcomponent {
    char *value;                           /* Subcomponent value */
    size_t length;                         /* Value length */
};

/**
 * HL7 Component
 * Example: In "PID|1||12345^JONES^MICHAEL", "JONES" is a component
 */
struct hl7_component {
    hl7_subcomponent_t *subcomponents;     /* Array of subcomponents */
    size_t num_subcomponents;              /* Number of subcomponents */

    /* Fast access to raw value (when no subcomponents) */
    char *value;                           /* Component value */
    size_t length;                         /* Value length */
};

/**
 * HL7 Field
 * Example: In "PID|1||12345^JONES", "12345^JONES" is a field
 */
struct hl7_field {
    hl7_component_t *components;           /* Array of components */
    size_t num_components;                 /* Number of components */

    /* Fast access to raw value (when no components) */
    char *value;                           /* Field value */
    size_t length;                         /* Value length */

    /* Repeating fields (separated by ~) */
    struct hl7_field *repetitions;         /* Array of repetitions */
    size_t num_repetitions;                /* Number of repetitions */
};

/**
 * HL7 Segment
 * Example: "PID|1||12345^JONES|..." - entire line is a segment
 */
struct hl7_segment {
    char id[HL7_MAX_SEGMENT_ID_LEN + 1];   /* Segment ID (e.g., "PID", "MSH") */

    hl7_field_t *fields;                   /* Array of fields */
    size_t num_fields;                     /* Number of fields */

    /* Raw segment data for reference */
    char *raw_data;                        /* Original segment text */
    size_t raw_length;                     /* Length of raw data */

    /* Linked list for easy traversal */
    struct hl7_segment *next;              /* Next segment in message */
};

/**
 * HL7 Message Type
 */
typedef enum {
    HL7_MSG_UNKNOWN = 0,
    HL7_MSG_ADT,        /* Admission/Discharge/Transfer */
    HL7_MSG_ORU,        /* Observation Result */
    HL7_MSG_ORM,        /* Order Message */
    HL7_MSG_DFT,        /* Detailed Financial Transaction */
    HL7_MSG_SIU,        /* Scheduling Information Unsolicited */
    HL7_MSG_MDM,        /* Medical Document Management */
    HL7_MSG_ACK,        /* General Acknowledgment */
    HL7_MSG_QRY,        /* Query */
    HL7_MSG_RAS,        /* Pharmacy/Treatment Administration */
    HL7_MSG_BAR,        /* Billing Account Record */
} hl7_message_type_t;

/**
 * HL7 Message Version
 */
typedef enum {
    HL7_VERSION_UNKNOWN = 0,
    HL7_VERSION_2_1,
    HL7_VERSION_2_2,
    HL7_VERSION_2_3,
    HL7_VERSION_2_3_1,
    HL7_VERSION_2_4,
    HL7_VERSION_2_5,
    HL7_VERSION_2_5_1,
    HL7_VERSION_2_6,
    HL7_VERSION_2_7,
    HL7_VERSION_2_8,
} hl7_version_t;

/**
 * HL7 Encoding Characters
 */
typedef struct {
    char field_separator;           /* Usually | */
    char component_separator;       /* Usually ^ */
    char repetition_separator;      /* Usually ~ */
    char escape_character;          /* Usually \ */
    char subcomponent_separator;    /* Usually & */
} hl7_encoding_chars_t;

/**
 * HL7 Message
 * Complete HL7 v2.x message with all segments
 */
struct hl7_message {
    uint64_t msg_id;                       /* Unique message ID (database assigned) */
    time_t timestamp;                      /* Message timestamp */

    /* Message type and version */
    hl7_message_type_t msg_type;           /* Message type */
    char msg_type_str[HL7_MAX_MESSAGE_TYPE_LEN]; /* e.g., "ADT^A01" */
    hl7_version_t version;                 /* HL7 version */

    /* Encoding characters from MSH-2 */
    hl7_encoding_chars_t encoding;

    /* Segments */
    hl7_segment_t *segments;               /* Linked list of segments */
    size_t num_segments;                   /* Number of segments */

    /* Fast access to common segments */
    hl7_segment_t *msh;                    /* Message Header (always present) */
    hl7_segment_t *pid;                    /* Patient ID (if present) */

    /* Raw message data */
    char *raw_message;                     /* Original message text */
    size_t raw_length;                     /* Length of raw message */

    /* Parsing metadata */
    int parse_errors;                      /* Number of parse errors */
    char *error_message;                   /* Error description if any */

    /* Memory pool for efficient allocation */
    void *memory_pool;                     /* Memory pool (memory_pool_t*) */
};

/**
 * HL7 Parse Options
 */
typedef struct {
    int strict_mode;                       /* Strict validation */
    int allow_non_standard_delimiters;     /* Allow custom delimiters */
    int validate_structure;                /* Validate segment structure */
    int trim_whitespace;                   /* Trim field whitespace */
    int decode_escape_sequences;           /* Decode \X\ escape sequences */
} hl7_parse_options_t;

/**
 * HL7 Field Path
 * Used for querying specific fields like "PID-3" or "OBX-5.1.2"
 */
typedef struct {
    char segment_id[HL7_MAX_SEGMENT_ID_LEN + 1];
    int segment_index;                     /* 0 = first occurrence */
    int field_index;                       /* 1-based (MSH-1 is field separator) */
    int component_index;                   /* 0-based, -1 = whole field */
    int subcomponent_index;                /* 0-based, -1 = whole component */
    int repetition_index;                  /* 0-based, -1 = first repetition */
} hl7_field_path_t;

#endif /* HL7DB_HL7_TYPES_H */
