# HL7DB API User Guide

This guide shows how to integrate the HL7DB API into your application for storing, querying, and managing HL7 v2.x messages.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Core Concepts](#core-concepts)
3. [API Overview](#api-overview)
4. [Common Use Cases](#common-use-cases)
5. [Query Language Reference](#query-language-reference)
6. [Thread Safety](#thread-safety)
7. [Error Handling](#error-handling)
8. [Best Practices](#best-practices)

## Quick Start

### Minimal Example

```c
#include "query/hl7sql_parser.h"
#include "query/hl7sql_channel_manager.h"
#include "query/hl7sql_result.h"
#include "util/logger.h"

int main(void) {
    // Initialize logger
    logger_init_default();

    // Create channel manager (main API entry point)
    hl7sql_channel_manager_t *mgr = hl7sql_channel_manager_create();

    // Create a channel for ADT messages
    hl7sql_result_t *result = hl7sql_query(mgr, "CREATE CHANNEL adt_messages");
    hl7sql_result_destroy(result);

    // Insert an HL7 message
    const char *insert_query =
        "INSERT MESSAGE INTO adt_messages "
        "'MSH|^~\\&|SENDER|FAC|RECEIVER|FAC|20230615083000||ADT^A01|MSG001|P|2.5\r"
        "PID|1||123456789^^^FAC^MR||JONES^WILLIAM^A||19800515|M'";

    result = hl7sql_query(mgr, insert_query);
    if (result->error_code != 0) {
        printf("Error: %s\n", result->error_message);
    }
    hl7sql_result_destroy(result);

    // Query messages
    result = hl7sql_query(mgr, "SELECT * FROM adt_messages WHERE segment.PID[3] = '123456789'");
    printf("Found %zu messages\n", result->num_rows);
    hl7sql_result_print(result);
    hl7sql_result_destroy(result);

    // Cleanup
    hl7sql_channel_manager_destroy(mgr);
    logger_cleanup();

    return 0;
}
```

### Compilation

```bash
# Link against HL7DB libraries
gcc -o myapp myapp.c \
    -I/path/to/HL7DB/include \
    -L/path/to/HL7DB/lib \
    -lhl7db \
    -lpthread
```

## Core Concepts

### 1. Channel Manager

The **channel manager** is your main entry point to the HL7DB API. It manages multiple named channels, each containing a separate message store.

```c
hl7sql_channel_manager_t *mgr = hl7sql_channel_manager_create();
// ... use the manager ...
hl7sql_channel_manager_destroy(mgr);
```

### 2. Channels

**Channels** are named containers for HL7 messages. Think of them as tables in a traditional database or topics in a message queue.

Use cases:
- Separate message types: `adt_messages`, `lab_results`, `orders`
- Separate facilities: `hospital_a`, `hospital_b`
- Separate workflows: `incoming`, `validated`, `processed`

### 3. HL7SQL Query Language

**HL7SQL** is a SQL-like language designed specifically for HL7 messages. It supports:
- Channel operations: `CREATE CHANNEL`, `DROP CHANNEL`
- Message operations: `INSERT MESSAGE`, `SELECT`, `POP MESSAGE`
- Field-level querying: `WHERE segment.PID[3] = '123456789'`
- Wildcard queries: `SELECT * FROM *` (all channels)

### 4. Query Results

All queries return an `hl7sql_result_t` structure containing:
- Row data (for SELECT queries)
- Error codes and messages
- Row/column metadata

Always destroy results when done:
```c
hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM messages");
// ... use result ...
hl7sql_result_destroy(result);
```

## API Overview

### Channel Management

#### Create Channel
```c
hl7sql_result_t *result = hl7sql_query(mgr, "CREATE CHANNEL channel_name");

// Or use the C API directly:
int rc = hl7sql_channel_manager_create_channel(mgr, "channel_name");
// Returns: 0 = success, -1 = already exists, -2 = allocation error
```

#### Drop Channel
```c
hl7sql_result_t *result = hl7sql_query(mgr, "DROP CHANNEL channel_name");

// Or use the C API:
int rc = hl7sql_channel_manager_drop_channel(mgr, "channel_name");
// Returns: 0 = success, -1 = not found, -2 = not empty
```

**Note**: Channels must be empty before dropping.

#### List Channels
```c
size_t num_channels;
char **channels = hl7sql_channel_manager_get_all_channels(mgr, &num_channels);

for (size_t i = 0; i < num_channels; i++) {
    printf("Channel: %s\n", channels[i]);
}

hl7sql_channel_manager_free_channel_names(channels, num_channels);
```

#### Check Channel Status
```c
// Check if channel exists
if (hl7sql_channel_manager_has_channel(mgr, "adt_messages")) {
    printf("Channel exists\n");
}

// Check if channel is empty
if (hl7sql_channel_manager_is_empty(mgr, "adt_messages")) {
    printf("Channel is empty\n");
}
```

### Message Operations

#### Insert Messages
```c
// Build insert query with HL7 message
char query[4096];
snprintf(query, sizeof(query),
    "INSERT MESSAGE INTO adt_messages '%s'",
    hl7_message_string);

hl7sql_result_t *result = hl7sql_query(mgr, query);

if (result->error_code != 0) {
    fprintf(stderr, "Insert failed: %s\n", result->error_message);
}

hl7sql_result_destroy(result);
```

**Important**: HL7 messages in INSERT queries must:
- Be enclosed in single quotes
- Use `\r` for segment separators (not `\n`)
- Escape special characters properly

#### Query Messages

##### Select All Messages from Channel
```c
hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM adt_messages");

printf("Found %zu messages\n", result->num_rows);
for (size_t i = 0; i < result->num_rows; i++) {
    // Access row data
    hl7sql_row_t *row = &result->rows[i];
    printf("Message ID: %lu\n", row->msg_id);
}

hl7sql_result_destroy(result);
```

##### Select from All Channels
```c
// Use wildcard to query all channels
hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM *");
```

##### Select with WHERE Clause
```c
// Query by patient ID
hl7sql_result_t *result = hl7sql_query(mgr,
    "SELECT * FROM adt_messages WHERE segment.PID[3] = '123456789'");

// Query by message type
result = hl7sql_query(mgr,
    "SELECT * FROM * WHERE segment.MSH[9] = 'ADT^A01'");

// Query by patient name (partial match)
result = hl7sql_query(mgr,
    "SELECT * FROM adt_messages WHERE segment.PID[5] = 'JONES'");
```

#### Pop Messages (Queue Semantics)

Remove and return the oldest message from a channel (FIFO):

```c
hl7sql_result_t *result = hl7sql_query(mgr, "POP MESSAGE FROM adt_messages");

if (result->num_rows > 0) {
    // Process the popped message
    hl7sql_row_t *row = &result->rows[0];
    printf("Popped message ID: %lu\n", row->msg_id);
    // Message is now removed from the channel
}

hl7sql_result_destroy(result);
```

### Working with Query Results

#### Result Structure
```c
typedef struct {
    hl7sql_row_t *rows;        // Array of result rows
    size_t num_rows;           // Number of rows returned
    size_t num_cols;           // Number of columns (usually 2: msg_id, raw_message)
    int error_code;            // 0 = success, non-zero = error
    char *error_message;       // Human-readable error message (NULL if no error)
} hl7sql_result_t;
```

#### Row Structure
```c
typedef struct {
    uint64_t msg_id;           // Unique message ID
    char *raw_message;         // Raw HL7 message string
    hl7_message_t *msg;        // Parsed HL7 message (may be NULL)
} hl7sql_row_t;
```

#### Accessing Result Data
```c
hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM adt_messages");

if (result->error_code != 0) {
    fprintf(stderr, "Query error: %s\n", result->error_message);
    hl7sql_result_destroy(result);
    return -1;
}

for (size_t i = 0; i < result->num_rows; i++) {
    hl7sql_row_t *row = &result->rows[i];

    printf("Message ID: %lu\n", row->msg_id);
    printf("Raw: %s\n", row->raw_message);

    // Access parsed message if available
    if (row->msg) {
        printf("Type: %s\n", row->msg->msg_type_str);
        printf("Segments: %zu\n", row->msg->num_segments);
    }
}

hl7sql_result_destroy(result);
```

#### Print Results (Debug)
```c
hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM adt_messages");
hl7sql_result_print(result);  // Prints formatted table to stdout
hl7sql_result_destroy(result);
```

## Common Use Cases

### Use Case 1: HL7 Interface Engine

Process incoming HL7 messages, validate, and route to appropriate channels:

```c
void process_incoming_message(hl7sql_channel_manager_t *mgr, const char *raw_msg) {
    // Parse message to determine type
    hl7_message_t *msg = hl7_parse(raw_msg, NULL);
    if (!msg) {
        fprintf(stderr, "Failed to parse HL7 message\n");
        return;
    }

    // Route based on message type
    const char *msg_type = hl7_message_get_field(msg, "MSH-9");
    char query[4096];

    if (strncmp(msg_type, "ADT", 3) == 0) {
        snprintf(query, sizeof(query), "INSERT MESSAGE INTO adt_messages '%s'", raw_msg);
    } else if (strncmp(msg_type, "ORU", 3) == 0) {
        snprintf(query, sizeof(query), "INSERT MESSAGE INTO lab_results '%s'", raw_msg);
    } else if (strncmp(msg_type, "ORM", 3) == 0) {
        snprintf(query, sizeof(query), "INSERT MESSAGE INTO orders '%s'", raw_msg);
    } else {
        snprintf(query, sizeof(query), "INSERT MESSAGE INTO unknown '%s'", raw_msg);
    }

    hl7sql_result_t *result = hl7sql_query(mgr, query);
    if (result->error_code != 0) {
        fprintf(stderr, "Failed to store message: %s\n", result->error_message);
    }
    hl7sql_result_destroy(result);
    hl7_message_destroy(msg);
}
```

### Use Case 2: Patient Data Lookup

Find all messages for a specific patient:

```c
void find_patient_messages(hl7sql_channel_manager_t *mgr, const char *patient_id) {
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT * FROM * WHERE segment.PID[3] = '%s'", patient_id);

    hl7sql_result_t *result = hl7sql_query(mgr, query);

    printf("Found %zu messages for patient %s:\n", result->num_rows, patient_id);
    for (size_t i = 0; i < result->num_rows; i++) {
        hl7sql_row_t *row = &result->rows[i];
        if (row->msg) {
            const char *msg_type = hl7_message_get_field(row->msg, "MSH-9");
            const char *timestamp = hl7_message_get_field(row->msg, "MSH-7");
            printf("  %s - %s\n", timestamp ? timestamp : "?", msg_type ? msg_type : "?");
        }
    }

    hl7sql_result_destroy(result);
}
```

### Use Case 3: Message Queue Processing

Use channels as FIFO queues with POP semantics:

```c
void process_queue(hl7sql_channel_manager_t *mgr) {
    while (1) {
        // Pop oldest message
        hl7sql_result_t *result = hl7sql_query(mgr, "POP MESSAGE FROM incoming");

        if (result->num_rows == 0) {
            // Queue is empty
            hl7sql_result_destroy(result);
            break;
        }

        // Process message
        hl7sql_row_t *row = &result->rows[0];
        printf("Processing message ID %lu\n", row->msg_id);

        // Do your business logic here...

        hl7sql_result_destroy(result);
    }
}
```

### Use Case 4: Multi-Channel Workflow

Implement a validation workflow with multiple stages:

```c
void setup_workflow(hl7sql_channel_manager_t *mgr) {
    hl7sql_result_destroy(hl7sql_query(mgr, "CREATE CHANNEL incoming"));
    hl7sql_result_destroy(hl7sql_query(mgr, "CREATE CHANNEL validated"));
    hl7sql_result_destroy(hl7sql_query(mgr, "CREATE CHANNEL errors"));
}

void validate_and_route(hl7sql_channel_manager_t *mgr) {
    // Pop from incoming
    hl7sql_result_t *result = hl7sql_query(mgr, "POP MESSAGE FROM incoming");
    if (result->num_rows == 0) {
        hl7sql_result_destroy(result);
        return;
    }

    hl7sql_row_t *row = &result->rows[0];

    // Validate
    int valid = hl7_validator_validate(row->msg);

    // Route based on validation
    char insert_query[4096];
    if (valid == 0) {
        snprintf(insert_query, sizeof(insert_query),
            "INSERT MESSAGE INTO validated '%s'", row->raw_message);
    } else {
        snprintf(insert_query, sizeof(insert_query),
            "INSERT MESSAGE INTO errors '%s'", row->raw_message);
    }

    hl7sql_result_destroy(result);
    result = hl7sql_query(mgr, insert_query);
    hl7sql_result_destroy(result);
}
```

## Query Language Reference

### CREATE CHANNEL
```sql
CREATE CHANNEL channel_name
```
Creates a new empty channel with the given name.

### DROP CHANNEL
```sql
DROP CHANNEL channel_name
```
Deletes an empty channel. Fails if channel contains messages.

### INSERT MESSAGE
```sql
INSERT MESSAGE INTO channel_name 'HL7_MESSAGE_HERE'
```
Parses and stores an HL7 message in the specified channel.

**Example**:
```sql
INSERT MESSAGE INTO adt_messages
'MSH|^~\&|APP|FAC|APP2|FAC2|20230615|SEC|ADT^A01|MSG001|P|2.5\r
PID|1||123456789^^^FAC^MR||DOE^JOHN^||19800101|M'
```

### SELECT
```sql
SELECT * FROM channel_name [WHERE condition]
SELECT * FROM * [WHERE condition]
```

**Examples**:
```sql
-- All messages from a channel
SELECT * FROM adt_messages

-- All messages from all channels
SELECT * FROM *

-- Messages matching a condition
SELECT * FROM adt_messages WHERE segment.PID[3] = '123456789'

-- Query across all channels
SELECT * FROM * WHERE segment.MSH[9] = 'ORU^R01'
```

### POP MESSAGE
```sql
POP MESSAGE FROM channel_name
```
Removes and returns the oldest message (FIFO). Message is deleted from the channel.

### WHERE Clause Field References

Field references use the format: `segment.SEGMENT[field_number]`

**Common examples**:
- `segment.PID[3]` - Patient ID (PID-3)
- `segment.PID[5]` - Patient Name (PID-5)
- `segment.MSH[9]` - Message Type (MSH-9)
- `segment.MSH[7]` - Message Timestamp (MSH-7)
- `segment.OBX[3]` - Observation Identifier (OBX-3)
- `segment.OBX[5]` - Observation Value (OBX-5)

**Operators**:
- `=` - Equality (exact match or substring match)
- Currently, equality is the primary supported operator

## Thread Safety

### Channel Manager
The channel manager uses a **read-write lock** for channel CRUD operations:
- Multiple threads can query existing channels concurrently
- Channel creation/deletion requires exclusive access

### Individual Channels
Each channel (message store) has its own **mutex lock**:
- Thread-safe for concurrent reads and writes
- Safe to query from multiple threads simultaneously

### Best Practices for Multi-Threading

```c
// Good: Each thread can safely query concurrently
void *worker_thread(void *arg) {
    hl7sql_channel_manager_t *mgr = (hl7sql_channel_manager_t *)arg;

    hl7sql_result_t *result = hl7sql_query(mgr,
        "SELECT * FROM adt_messages WHERE segment.PID[3] = '123'");

    // Process result...

    hl7sql_result_destroy(result);
    return NULL;
}

// Good: Create channels before spawning threads
void setup(void) {
    hl7sql_channel_manager_t *mgr = hl7sql_channel_manager_create();

    // Create all channels first
    hl7sql_result_destroy(hl7sql_query(mgr, "CREATE CHANNEL adt_messages"));
    hl7sql_result_destroy(hl7sql_query(mgr, "CREATE CHANNEL lab_results"));

    // Now spawn worker threads...
}
```

## Error Handling

### Check Result Error Code
```c
hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM nonexistent");

if (result->error_code != 0) {
    fprintf(stderr, "Query failed: %s (code %d)\n",
            result->error_message, result->error_code);
    hl7sql_result_destroy(result);
    return -1;
}

// Success - process results
hl7sql_result_destroy(result);
```

### Common Error Codes
- `0` - Success
- Non-zero - Error (check `error_message` for details)

### NULL Results
A NULL result indicates a critical failure (allocation error):
```c
hl7sql_result_t *result = hl7sql_query(mgr, query);
if (!result) {
    fprintf(stderr, "Critical error: NULL result\n");
    return -1;
}
```

### Parse Errors
Invalid HL7 messages will fail to insert:
```c
hl7sql_result_t *result = hl7sql_query(mgr,
    "INSERT MESSAGE INTO channel 'INVALID HL7'");

if (result->error_code != 0) {
    fprintf(stderr, "Parse error: %s\n", result->error_message);
}
```

## Best Practices

### 1. Always Destroy Results
```c
// Good
hl7sql_result_t *result = hl7sql_query(mgr, query);
// ... use result ...
hl7sql_result_destroy(result);

// Bad - memory leak
result = hl7sql_query(mgr, query);
// forgot to destroy!
```

### 2. Check Error Codes
```c
// Good
hl7sql_result_t *result = hl7sql_query(mgr, query);
if (result->error_code != 0) {
    handle_error(result->error_message);
}

// Bad - ignoring errors
hl7sql_result_t *result = hl7sql_query(mgr, query);
// assuming it worked...
```

### 3. Use Channels to Organize Messages
```c
// Good - organized by type
CREATE CHANNEL adt_messages
CREATE CHANNEL oru_messages
CREATE CHANNEL orm_messages

// Less ideal - everything in one channel
CREATE CHANNEL all_messages
```

### 4. Validate Before Inserting
```c
// Good
hl7_message_t *msg = hl7_parse(raw_message, NULL);
if (msg && hl7_validator_validate(msg) == 0) {
    // Message is valid, insert it
    snprintf(query, sizeof(query),
        "INSERT MESSAGE INTO channel '%s'", raw_message);
    hl7sql_query(mgr, query);
}
hl7_message_destroy(msg);
```

### 5. Escape Query Strings Properly
```c
// HL7 messages use special characters
// Single quotes should be escaped in dynamic queries
// For now, ensure your HL7 messages don't contain single quotes
// or use parameterized queries when available
```

### 6. Initialize Logger for Debugging
```c
#include "util/logger.h"

int main(void) {
    logger_init_default();
    logger_set_level(LOG_DEBUG);  // or LOG_INFO, LOG_WARN

    // Your code here...

    logger_cleanup();
}
```

### 7. Resource Cleanup
```c
void cleanup_example(void) {
    hl7sql_channel_manager_t *mgr = hl7sql_channel_manager_create();

    // Do work...

    // Clean up in reverse order
    hl7sql_channel_manager_destroy(mgr);  // Destroys all channels too
    logger_cleanup();
}
```

## Additional Resources

- **HL7 Parser API**: See `include/hl7/hl7_parser.h` and `include/hl7/hl7_message.h`
- **Field Extraction**: Use `hl7_message_get_field(msg, "PID-3")` for direct access
- **Segment Access**: Use `hl7_message_get_segment(msg, "PID", 0)` for segment-level access
- **Validation**: See `include/hl7/hl7_validator.h` for message validation

## Example: Complete Application

```c
#include "query/hl7sql_parser.h"
#include "query/hl7sql_channel_manager.h"
#include "hl7/hl7_parser.h"
#include "hl7/hl7_validator.h"
#include "util/logger.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    logger_init_default();
    logger_set_level(LOG_INFO);

    // Setup
    hl7sql_channel_manager_t *mgr = hl7sql_channel_manager_create();
    if (!mgr) {
        fprintf(stderr, "Failed to create channel manager\n");
        return 1;
    }

    // Create channels
    hl7sql_result_destroy(hl7sql_query(mgr, "CREATE CHANNEL admissions"));
    hl7sql_result_destroy(hl7sql_query(mgr, "CREATE CHANNEL lab_results"));

    // Insert ADT message
    const char *adt_msg =
        "MSH|^~\\&|APP|FAC|RECV|FAC2|20230615083000||ADT^A01|MSG001|P|2.5\r"
        "PID|1||123456789^^^FAC^MR||SMITH^JOHN^A||19750101|M";

    char insert_query[2048];
    snprintf(insert_query, sizeof(insert_query),
        "INSERT MESSAGE INTO admissions '%s'", adt_msg);

    hl7sql_result_t *result = hl7sql_query(mgr, insert_query);
    if (result->error_code == 0) {
        printf("Successfully inserted ADT message\n");
    }
    hl7sql_result_destroy(result);

    // Query by patient ID
    result = hl7sql_query(mgr,
        "SELECT * FROM admissions WHERE segment.PID[3] = '123456789'");

    printf("\nQuery results: %zu messages found\n", result->num_rows);
    hl7sql_result_print(result);
    hl7sql_result_destroy(result);

    // Cleanup
    hl7sql_channel_manager_destroy(mgr);
    logger_cleanup();

    return 0;
}
```

## Summary

The HL7DB API provides:
- Simple channel-based message organization
- SQL-like query language for HL7 messages
- Thread-safe concurrent access
- Field-level querying without denormalization
- Queue semantics with POP operations

Key functions to remember:
- `hl7sql_channel_manager_create()` - Initialize API
- `hl7sql_query()` - Execute any HL7SQL query
- `hl7sql_result_destroy()` - Free query results
- `hl7sql_channel_manager_destroy()` - Cleanup

For questions or issues, refer to the main README.md or source code documentation.
