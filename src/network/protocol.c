/**
 * protocol.c - HL7DB Binary Protocol Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include "network/protocol.h"
#include "util/logger.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

/**
 * Read exactly n bytes from file descriptor
 *
 * Returns: 0 on success, -1 on error, -2 on connection closed
 */
static int read_exact(int fd, void *buf, size_t n) {
    size_t total = 0;
    char *ptr = (char *)buf;

    while (total < n) {
        ssize_t nread = read(fd, ptr + total, n - total);

        if (nread < 0) {
            if (errno == EINTR) {
                continue;  /* Interrupted by signal, retry */
            }
            LOG_ERROR("read() failed: %s", strerror(errno));
            return -1;
        }

        if (nread == 0) {
            /* Connection closed */
            return -2;
        }

        total += nread;
    }

    return 0;
}

/**
 * Write exactly n bytes to file descriptor
 *
 * Returns: 0 on success, -1 on error
 */
static int write_exact(int fd, const void *buf, size_t n) {
    size_t total = 0;
    const char *ptr = (const char *)buf;

    while (total < n) {
        ssize_t nwritten = write(fd, ptr + total, n - total);

        if (nwritten < 0) {
            if (errno == EINTR) {
                continue;  /* Interrupted by signal, retry */
            }
            LOG_ERROR("write() failed: %s", strerror(errno));
            return -1;
        }

        total += nwritten;
    }

    return 0;
}

int protocol_read_request(int fd, protocol_request_t *request) {
    if (!request) {
        return -1;
    }

    /* Read length field (4 bytes, network byte order) */
    uint32_t length_net;
    int rc = read_exact(fd, &length_net, sizeof(length_net));
    if (rc != 0) {
        return rc;  /* -1 = error, -2 = connection closed */
    }

    request->length = ntohl(length_net);

    /* Validate length */
    if (request->length == 0) {
        LOG_WARN("Received empty query");
        return -1;
    }

    if (request->length > PROTOCOL_MAX_QUERY_SIZE) {
        LOG_ERROR("Query too large: %u bytes (max %u)",
                  request->length, PROTOCOL_MAX_QUERY_SIZE);
        return -1;
    }

    /* Allocate buffer for query (+1 for null terminator) */
    request->query = malloc(request->length + 1);
    if (!request->query) {
        LOG_ERROR("Failed to allocate query buffer");
        return -1;
    }

    /* Read query string */
    rc = read_exact(fd, request->query, request->length);
    if (rc != 0) {
        free(request->query);
        request->query = NULL;
        return rc;
    }

    /* Null-terminate the query */
    request->query[request->length] = '\0';

    LOG_DEBUG("Received request: %u bytes", request->length);
    return 0;
}

int protocol_write_response(int fd, const protocol_response_t *response) {
    if (!response) {
        return -1;
    }

    /* Write status (4 bytes, network byte order) */
    uint32_t status_net = htonl(response->status);
    if (write_exact(fd, &status_net, sizeof(status_net)) != 0) {
        return -1;
    }

    /* Write length (4 bytes, network byte order) */
    uint32_t length_net = htonl(response->length);
    if (write_exact(fd, &length_net, sizeof(length_net)) != 0) {
        return -1;
    }

    /* Write data (if any) */
    if (response->length > 0 && response->data) {
        if (write_exact(fd, response->data, response->length) != 0) {
            return -1;
        }
    }

    LOG_DEBUG("Sent response: status=%d, length=%u",
              response->status, response->length);
    return 0;
}

protocol_response_t *protocol_create_success_response(const char *data) {
    protocol_response_t *response = calloc(1, sizeof(protocol_response_t));
    if (!response) {
        LOG_ERROR("Failed to allocate response");
        return NULL;
    }

    response->status = PROTOCOL_STATUS_SUCCESS;

    if (data && *data) {
        response->length = strlen(data);
        response->data = strdup(data);
        if (!response->data) {
            LOG_ERROR("Failed to allocate response data");
            free(response);
            return NULL;
        }
    } else {
        response->length = 0;
        response->data = NULL;
    }

    return response;
}

protocol_response_t *protocol_create_error_response(protocol_status_t status,
                                                      const char *message) {
    protocol_response_t *response = calloc(1, sizeof(protocol_response_t));
    if (!response) {
        LOG_ERROR("Failed to allocate error response");
        return NULL;
    }

    response->status = status;

    if (message && *message) {
        response->length = strlen(message);
        response->data = strdup(message);
        if (!response->data) {
            LOG_ERROR("Failed to allocate error message");
            free(response);
            return NULL;
        }
    } else {
        response->length = 0;
        response->data = NULL;
    }

    return response;
}

void protocol_free_request(protocol_request_t *request) {
    if (request) {
        free(request->query);
        request->query = NULL;
        request->length = 0;
    }
}

void protocol_free_response(protocol_response_t *response) {
    if (response) {
        free(response->data);
        free(response);
    }
}
