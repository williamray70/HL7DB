# HL7DB - HL7 v2.x Database Server

A specialized database server built from the ground up in C, where HL7 v2.x messages are the fundamental storage unit. Features a SQL-like query language (HL7SQL) for field-level searching and retrieval of healthcare messages.

## Overview

HL7DB is designed specifically for storing, indexing, and querying HL7 v2.x messages (pipe-delimited format). Unlike traditional databases that force HL7 messages into relational tables or generic document stores, HL7DB treats messages as first-class citizens with native support for HL7's hierarchical structure (segments, fields, components, subcomponents).

### Key Features

- **Native HL7 v2.x Support**: Parse and store ADT, ORU, ORM, and other message types
- **HL7SQL Query Language**: SQL-like queries with HL7-specific extensions
- **Field-Level Indexing**: Fast lookups by patient ID, message type, timestamp, and custom fields
- **ACID Compliance**: Write-Ahead Log (WAL) ensures durability and crash recovery
- **High Performance**: Append-only storage optimized for HL7's immutable nature
- **Concurrent Access**: Support for 100+ simultaneous clients

## Architecture

```
Client Applications (HL7SQL queries)
         ↓
Network Layer (TCP server, binary protocol)
         ↓
Query Engine (HL7SQL parser & executor)
         ↓
HL7 Processing (parse pipe-delimited format)
         ↓
Index Manager (B-tree indexes)
         ↓
Storage Engine (append-only log + WAL)
```

### Core Components

1. **Storage Engine**: Append-only log with page-based storage, buffer pool, and WAL
2. **HL7 Parser**: Parses pipe-delimited messages into queryable hierarchical structures
3. **Index Manager**: B-tree based multi-index system for fast lookups
4. **Query Engine**: HL7SQL parser (Flex/Bison) and execution engine
5. **Network Layer**: Custom binary protocol over TCP with epoll/select
6. **Utilities**: Memory pool allocator, hash tables, logging, configuration

## Building

### Dependencies

- **Compiler**: GCC or Clang (C11 standard)
- **Build Tools**: GNU Make
- **Parser Generator**: Flex (lexer) and Bison (parser)
- **Libraries**: pthread
- **Optional**: Valgrind (testing), GDB (debugging)

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt-get install build-essential flex bison
```

### Build Commands

```bash
# Build server
make

# Build server and all tools
make full

# Run tests (when available)
make test

# Clean build artifacts
make clean

# Show help
make help
```

### Build Output

- `bin/hl7db` - Database server
- `bin/hl7db-cli` - Interactive CLI client (Phase 7)
- `bin/hl7db-import` - Bulk import tool (Phase 7)

## Usage (When Complete)

### Starting the Server

```bash
./bin/hl7db --config hl7db.conf
```

### HL7SQL Query Examples

```sql
-- Insert HL7 message
INSERT MESSAGE 'MSH|^~\&|SENDER|FACILITY|...\rPID|1||123456789|...';

-- Query by patient ID
SELECT * FROM messages WHERE segment.PID[3] = '123456789';

-- Query by message type and date range
SELECT msg_id, segment.MSH[7] AS timestamp
FROM messages
WHERE segment.MSH[9] = 'ADT^A01'
  AND segment.MSH[7] BETWEEN '20240101' AND '20241231';

-- Find lab results above threshold
SELECT * FROM messages
WHERE segment.OBX[3] LIKE '%GLUCOSE%'
  AND segment.OBX[5] > '200';

-- Count messages by type
SELECT segment.MSH[9] AS msg_type, COUNT(*)
FROM messages
GROUP BY msg_type;
```

## Configuration

Example `hl7db.conf`:

```ini
[server]
port = 7777
max_connections = 1000

[storage]
data_directory = /var/lib/hl7db/data
page_size = 4096
buffer_pool_size = 268435456  # 256MB

[wal]
wal_sync_mode = fsync
checkpoint_interval = 300  # seconds

[index]
auto_index_fields = PID.3,MSH.9,MSH.7

[logging]
log_level = INFO
log_file = /var/log/hl7db/server.log
```

## Development Status

### Current Phase: Phase 1 - Foundation & Utilities ✅

**Completed:**
- ✅ Project structure and build system
- ✅ Makefile with comprehensive build targets
- ✅ Logger utility with thread-safe logging
- ✅ Memory pool allocator for efficient allocation
- ✅ Linked list implementation
- ✅ Hash table implementation
- ✅ Configuration parser (INI format)

**Next Phase:** Phase 2 - HL7 Parser (Weeks 3-5)

### Roadmap

- [x] **Phase 1**: Foundation & Utilities (Weeks 1-2)
- [ ] **Phase 2**: HL7 Parser (Weeks 3-5)
- [ ] **Phase 3**: Storage Engine (Weeks 6-9)
- [ ] **Phase 4**: Indexing System (Weeks 10-13)
- [ ] **Phase 5**: Query Engine (Weeks 14-18)
- [ ] **Phase 6**: Network Layer (Weeks 19-20)
- [ ] **Phase 7**: CLI Tools & Integration (Weeks 21-22)
- [ ] **Phase 8**: Advanced Features & Hardening (Weeks 23-25)

## Project Structure

```
HL7DB/
├── Makefile              # Build system
├── README.md            # This file
├── src/
│   ├── main.c           # Entry point (Phase 7)
│   ├── util/            # Utility libraries ✅
│   │   ├── logger.c/h
│   │   ├── memory.c/h
│   │   ├── list.c/h
│   │   ├── hashmap.c/h
│   │   └── config.c/h
│   ├── hl7/             # HL7 processing (Phase 2)
│   ├── storage/         # Storage engine (Phase 3)
│   ├── index/           # Indexing system (Phase 4)
│   ├── query/           # Query engine (Phase 5)
│   └── network/         # Network layer (Phase 6)
├── include/             # Header files
├── tests/               # Test suite
├── tools/               # CLI tools (Phase 7)
└── docs/                # Documentation
```

## Performance Targets

### Minimum Viable Product
- 100+ messages/sec insertion
- <50ms query latency (p95)
- 10+ concurrent clients
- Crash recovery without data loss

### Production Ready
- 1000+ messages/sec insertion
- <10ms query latency (p95)
- 100+ concurrent clients
- Comprehensive security (auth, TLS)

## Contributing

This is currently a development project. Contributions welcome once core functionality is implemented.

## License

TBD

## References

- [HL7 Version 2.x Specification](https://www.hl7.org/implement/standards/product_brief.cfm?product_id=185)
- [HL7 Message Structure Documentation](https://www.hl7.org/documentcenter/public_temp_BCFAFA9D-1C23-BA17-0CE03F7FD0F4AC64/wg/vocab/HL7Connect-Master-Message-Structure-Specification.pdf)

## Contact

Project maintained as part of healthcare interoperability infrastructure development.
