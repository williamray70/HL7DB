/**
 * hl7sql_parser.c - HL7SQL Query Parser Implementation
 */

#define _GNU_SOURCE

#include "query/hl7sql_parser.h"
#include "query/hl7sql_executor.h"
#include "util/logger.h"
#include <stdlib.h>
#include <string.h>

/* External Flex/Bison functions */
extern int yyparse(ast_node_t **result);
extern void yy_scan_string(const char *str);
extern void yy_delete_buffer(void *buffer);
extern void *yy_scan_buffer(char *base, size_t size);

/**
 * Parse an HL7SQL query string into AST
 */
ast_node_t *hl7sql_parse_query(const char *query) {
    if (!query) {
        LOG_ERROR("NULL query string");
        return NULL;
    }

    LOG_DEBUG("Parsing query: %s", query);

    /* Create a mutable copy of the query (Flex requires it) */
    size_t len = strlen(query);
    char *query_buf = malloc(len + 2);  /* +2 for Flex's YY_END_OF_BUFFER_CHAR */
    if (!query_buf) {
        LOG_ERROR("Failed to allocate query buffer");
        return NULL;
    }

    memcpy(query_buf, query, len);
    query_buf[len] = '\0';
    query_buf[len + 1] = '\0';  /* Flex requires double null termination */

    /* Set up Flex to scan our string */
    void *buffer = yy_scan_buffer(query_buf, len + 2);
    if (!buffer) {
        LOG_ERROR("Failed to create scan buffer");
        free(query_buf);
        return NULL;
    }

    /* Parse the query */
    ast_node_t *ast = NULL;
    LOG_DEBUG("Calling yyparse...");
    int parse_result = yyparse(&ast);
    LOG_DEBUG("yyparse returned: %d, ast=%p", parse_result, (void*)ast);

    /* Clean up */
    yy_delete_buffer(buffer);
    free(query_buf);

    if (parse_result != 0) {
        LOG_ERROR("Parse error in query (code %d)", parse_result);
        if (ast) {
            ast_destroy(ast);
        }
        return NULL;
    }

    if (!ast) {
        LOG_ERROR("Parser returned NULL AST");
        return NULL;
    }

    LOG_DEBUG("Successfully parsed query, AST type=%d", ast->type);

    return ast;
}

/**
 * Execute an HL7SQL query string
 */
hl7sql_result_t *hl7sql_query(hl7sql_queue_manager_t *mgr, const char *query) {
    if (!mgr || !query) {
        return hl7sql_result_create_error(-1, "Invalid channel manager or query");
    }

    /* Parse query into AST */
    ast_node_t *ast = hl7sql_parse_query(query);
    if (!ast) {
        return hl7sql_result_create_error(-2, "Parse error");
    }

    /* Execute AST */
    hl7sql_result_t *result = hl7sql_execute(mgr, ast);

    /* Clean up AST */
    ast_destroy(ast);

    return result;
}
