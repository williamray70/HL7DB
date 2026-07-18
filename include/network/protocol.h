/**
 * protocol.h - HL7DB Binary Protocol
 *
 * Simple binary protocol for client-server communication.
 * All integers are in network byte order (big-endian).
 *
 * Request Format:
 * ┌─────────────┬──────────────────┐
 * │ Length (4B) │ Query String     │
 * └─────────────┴──────────────────┘
 *
 * Response Format:
 * ┌─────────────┬─────────────┬──────────────────┐
 * │ Status (4B) │ Length (4B) │ Result Data      │
 * └─────────────┴─────────────┴──────────────────┘
 *
 * Status Codes:
 *   0x00000000 = Success
 *   0x00000001 = Query parse error
 *   0x00000002 = Execution error
 *   0x00000003 = Internal error
 */

#ifndef HL7DB_PROTOCOL_H
#define HL7DB_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/* Protocol constants */
#define PROTOCOL_MAX_QUERY_SIZE  (1024 * 1024)  /* 1 MB max query */
#define PROTOCOL_MAX_RESULT_SIZE (10 * 1024 * 1024) /* 10 MB max result */

/* Status codes */
typedef enum {
    PROTOCOL_STATUS_SUCCESS = 0,
    PROTOCOL_STATUS_PARSE_ERROR = 1,
    PROTOCOL_STATUS_EXEC_ERROR = 2,
    PROTOCOL_STATUS_INTERNAL_ERROR = 3
} protocol_status_t;

/* Request structure */
typedef struct {
    uint32_t length;      /* Length of query string */
    char *query;          /* Query string (null-terminated) */
} protocol_request_t;

/* Response structure */
typedef struct {
    protocol_status_t status;  /* Status code */
    uint32_t length;           /* Length of result data */
    char *data;                /* Result data (null-terminated) */
} protocol_response_t;

/**
 * Read a request from file descriptor
 *
 * Args:
 *   fd: File descriptor to read from
 *   request: Pointer to request structure to fill
 *
 * Returns:
 *   0 on success, -1 on error, -2 on connection closed
 *
 * Note: Caller must free request->query using free()
 */
int protocol_read_request(int fd, protocol_request_t *request);

/**
 * Write a response to file descriptor
 *
 * Args:
 *   fd: File descriptor to write to
 *   response: Response structure to send
 *
 * Returns:
 *   0 on success, -1 on error
 */
int protocol_write_response(int fd, const protocol_response_t *response);

/**
 * Create a success response
 *
 * Args:
 *   data: Result data (will be copied, can be NULL for empty result)
 *
 * Returns:
 *   Response structure (caller must free with protocol_free_response)
 */
protocol_response_t *protocol_create_success_response(const char *data);

/**
 * Create an error response
 *
 * Args:
 *   status: Error status code
 *   message: Error message (will be copied)
 *
 * Returns:
 *   Response structure (caller must free with protocol_free_response)
 */
protocol_response_t *protocol_create_error_response(protocol_status_t status,
                                                      const char *message);

/**
 * Free request structure
 *
 * Args:
 *   request: Request to free
 */
void protocol_free_request(protocol_request_t *request);

/**
 * Free response structure
 *
 * Args:
 *   response: Response to free
 */
void protocol_free_response(protocol_response_t *response);

#endif /* HL7DB_PROTOCOL_H */
