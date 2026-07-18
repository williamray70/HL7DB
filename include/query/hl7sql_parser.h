/**
 * hl7sql_parser.h - HL7SQL Query Parser
 *
 * Public API for parsing and executing HL7SQL queries.
 */

#ifndef HL7DB_HL7SQL_PARSER_H
#define HL7DB_HL7SQL_PARSER_H

#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_result.h"
#include "query/hl7sql_ast.h"

/**
 * Parse an HL7SQL query string into AST
 *
 * @param query SQL query string
 * @return AST node (caller must free with ast_destroy), or NULL on parse error
 */
ast_node_t *hl7sql_parse_query(const char *query);

/**
 * Execute an HL7SQL query string
 *
 * @param mgr Queue manager
 * @param query SQL query string
 * @return Query result (caller must free with hl7sql_result_destroy)
 *
 * This is the main entry point for executing queries. It parses the query
 * and executes it against the queue manager.
 */
hl7sql_result_t *hl7sql_query(hl7sql_queue_manager_t *mgr, const char *query);

#endif /* HL7DB_HL7SQL_PARSER_H */
