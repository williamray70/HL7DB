/**
 * hl7sql_executor.h - Query Execution Engine
 *
 * Executes parsed HL7SQL queries against queue manager.
 */

#ifndef HL7DB_HL7SQL_EXECUTOR_H
#define HL7DB_HL7SQL_EXECUTOR_H

#include "query/hl7sql_ast.h"
#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_result.h"

/**
 * Execute an HL7SQL query
 *
 * @param mgr Queue manager
 * @param ast Parsed query AST
 * @return Query result (caller must free with hl7sql_result_destroy)
 *
 * Supported query types:
 * - CREATE QUEUE: Create a new message queue
 * - DROP QUEUE: Delete an empty queue
 * - SELECT: Query messages with WHERE conditions
 * - INSERT: Parse and store HL7 messages
 * - POP MESSAGE: Remove and return oldest message (FIFO)
 */
hl7sql_result_t *hl7sql_execute(hl7sql_queue_manager_t *mgr, ast_node_t *ast);

#endif /* HL7DB_HL7SQL_EXECUTOR_H */
