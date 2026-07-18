# Channel History Feature

## Overview

HL7DB channels now include **automatic history tracking** for processed messages. When messages are popped from a channel queue (using `POP MESSAGE`), they are automatically moved to a history store instead of being deleted. This allows you to:

- Maintain an audit trail of processed messages
- Query previously processed messages
- Track message processing workflows
- Separate active queue from processed messages

## Architecture

Each channel maintains **two separate message stores**:

```
Channel: "adt_messages"
├── Active Queue (hl7sql_store_t)
│   ├── INSERT MESSAGE → appends here
│   ├── SELECT FROM adt_messages → queries here
│   └── POP MESSAGE → removes from here, moves to ↓
└── History (hl7sql_store_t)
    ├── Receives popped messages
    ├── SELECT FROM adt_messages.history → queries here
    └── CLEAR HISTORY adt_messages → clears history
```

### Storage Structure

```
data/channels/
├── adt_messages/
│   └── messages.dat          # Active queue
└── adt_messages.history/
    └── messages.dat          # History
```

## SQL Syntax

### Query History

```sql
-- Query all history
SELECT * FROM adt_messages.history

-- Query history with WHERE clause
SELECT * FROM adt_messages.history WHERE segment.PID[3] = '123456'

-- Count history messages
SELECT COUNT(*) FROM adt_messages.history

-- Filter by date
SELECT * FROM adt_messages.history WHERE segment.MSH[7] > '20240101'
```

### Clear History

```sql
-- Permanently delete all history for a channel
CLEAR HISTORY adt_messages
```

**Warning**: This operation is destructive and cannot be undone!

### Pop Message (Automatic History)

```sql
-- Pop message from queue (automatically moves to history)
POP MESSAGE FROM adt_messages
```

## API Reference

### C API Functions

#### Get History Store

```c
#include "query/hl7sql_channel_manager.h"

hl7sql_store_t *history = hl7sql_channel_manager_get_history(mgr, "channel_name");
```

#### Move Message to History

```c
int hl7sql_channel_manager_move_to_history(
    hl7sql_channel_manager_t *mgr,
    const char *channel_name,
    hl7_message_t *msg  // Manager takes ownership
);
```

#### Clear History

```c
int hl7sql_channel_manager_clear_history(
    hl7sql_channel_manager_t *mgr,
    const char *channel_name
);
```

#### Get History Size

```c
size_t size = hl7sql_channel_manager_get_history_size(mgr, "channel_name");
```

## Use Cases

### 1. Message Processing Pipeline

```c
while (true) {
    // Pop message from incoming queue
    result = hl7sql_query(mgr, "POP MESSAGE FROM incoming");
    if (result->num_rows == 0) {
        break;  // Queue empty
    }

    // Process message...
    process_hl7_message(result);

    // Message automatically moved to incoming.history
    hl7sql_result_destroy(result);
}

// Later: audit what was processed
result = hl7sql_query(mgr, "SELECT * FROM incoming.history");
printf("Processed %zu messages\n", result->num_rows);
```

### 2. Workflow Stages with History

```sql
-- Stage 1: Receive messages
INSERT MESSAGE INTO incoming 'MSH|...'

-- Stage 2: Pop and validate (moves to incoming.history)
POP MESSAGE FROM incoming

-- Stage 3: Query what was processed
SELECT * FROM incoming.history WHERE segment.MSH[9] = 'ADT^A01'

-- Stage 4: Clear old history (retention policy)
CLEAR HISTORY incoming
```

### 3. Error Recovery

```c
// Check if message was already processed
result = hl7sql_query(mgr,
    "SELECT * FROM orders.history WHERE segment.MSH[10] = 'MSG12345'");

if (result->num_rows > 0) {
    printf("Message MSG12345 already processed\n");
    // Skip reprocessing
} else {
    // Process new message
    result = hl7sql_query(mgr, "POP MESSAGE FROM orders");
    process(result);
}
```

### 4. Audit Trail

```sql
-- Find all messages processed for a patient
SELECT * FROM adt_messages.history
WHERE segment.PID[3] = '123456789'
ORDER BY segment.MSH[7]

-- Count messages processed per hour
SELECT SUBSTR(segment.MSH[7], 1, 10) AS hour, COUNT(*)
FROM lab_results.history
GROUP BY hour
```

## Persistence

History is persisted to disk (if persistence is enabled):

```c
// Enable persistence
persistence_config_t *config = persistence_config_create("./data/channels");
hl7sql_channel_manager_set_persistence(mgr, config);

// Both active queue and history persist across restarts
```

## Thread Safety

All history operations are thread-safe:

- **POP MESSAGE**: Acquires write lock on queue, then history
- **SELECT FROM history**: Acquires read lock on history store
- **CLEAR HISTORY**: Acquires write lock on history store
- **Concurrent access**: Multiple threads can safely pop/query/clear

## Performance

### Overhead

- **POP MESSAGE**: ~2-5 µs additional overhead to move to history vs. delete
- **History queries**: Same performance as regular channel queries
- **CLEAR HISTORY**: O(n) where n = number of history messages

### Storage

History messages are stored separately:
- Active queue: Fast FIFO operations
- History: Sequential append-only log
- No performance impact on active queue

## Limitations

1. **No automatic retention**: History grows indefinitely until manually cleared
2. **No history limits**: No max size limit (use `CLEAR HISTORY` periodically)
3. **No message recovery**: Cannot move messages back from history to queue
4. **No partial clear**: `CLEAR HISTORY` deletes all history, not selective

## Best Practices

### ✓ DO

1. **Implement retention policies**
   ```sql
   -- Clear history older than 30 days (implement in application)
   CLEAR HISTORY adt_messages
   ```

2. **Use history for auditing**
   ```sql
   SELECT * FROM orders.history WHERE segment.PID[3] = 'PATIENT_ID'
   ```

3. **Query before clearing**
   ```sql
   -- Check size before clearing
   SELECT COUNT(*) FROM logs.history
   CLEAR HISTORY logs
   ```

4. **Separate channels for retention**
   ```sql
   -- Short-term processing
   INSERT MESSAGE INTO temp_queue 'MSH|...'

   -- Long-term archival
   INSERT MESSAGE INTO archive 'MSH|...'
   ```

### ✗ DON'T

1. **Don't rely on history for data recovery**
   - History is for audit, not backup
   - Use proper backup strategies

2. **Don't query history in hot paths**
   - History queries are for analysis, not real-time processing
   - Use active queue for real-time operations

3. **Don't forget to clear history**
   - Implement retention policies
   - Monitor history size

## Examples

### Complete Workflow Example

```c
#include "query/hl7sql_parser.h"
#include "query/hl7sql_channel_manager.h"

int main(void) {
    hl7sql_channel_manager_t *mgr = hl7sql_channel_manager_create();

    // Create channel (automatically gets history support)
    hl7sql_query(mgr, "CREATE CHANNEL orders");

    // Insert messages
    hl7sql_query(mgr, "INSERT MESSAGE INTO orders 'MSH|...|MSG001|...'");
    hl7sql_query(mgr, "INSERT MESSAGE INTO orders 'MSH|...|MSG002|...'");
    hl7sql_query(mgr, "INSERT MESSAGE INTO orders 'MSH|...|MSG003|...'");

    // Process messages (moves to history)
    for (int i = 0; i < 3; i++) {
        hl7sql_result_t *msg = hl7sql_query(mgr, "POP MESSAGE FROM orders");
        printf("Processing: %s\n", msg->rows[0][1]);

        // Message now in orders.history
        hl7sql_result_destroy(msg);
    }

    // Query history
    hl7sql_result_t *history = hl7sql_query(mgr, "SELECT * FROM orders.history");
    printf("Processed %zu messages\n", history->num_rows);
    hl7sql_result_destroy(history);

    // Clear history (after archival or retention period)
    hl7sql_query(mgr, "CLEAR HISTORY orders");

    hl7sql_channel_manager_destroy(mgr);
    return 0;
}
```

## Testing

Run the history test suite:

```bash
./bin/test_history
```

This tests:
- History creation on channel creation
- Messages moving to history on POP
- Querying history with WHERE clauses
- Clearing history
- Verifying separation of active queue and history

## Migration

Existing channels are automatically upgraded:
- Channels created before this feature get empty history stores
- No migration required
- Backward compatible with existing code

## Future Enhancements

Planned improvements:

1. **Automatic retention policies**: Configure TTL for history
2. **History size limits**: Max messages in history
3. **Selective history clear**: Clear by date range or criteria
4. **History compression**: Compress old history messages
5. **History export**: Export history to external storage
6. **Statistics**: Per-channel history metrics

## See Also

- [USER_GUIDE.md](../USER_GUIDE.md) - Complete HL7SQL reference
- [README.md](../README.md) - Project overview
- [test_history.c](../test_history.c) - Example code
