/**
 * tcp_server.h - TCP Server for HL7DB
 *
 * Multi-threaded TCP server that accepts client connections and
 * processes HL7SQL queries.
 */

#ifndef HL7DB_TCP_SERVER_H
#define HL7DB_TCP_SERVER_H

#include "query/hl7sql_queue_manager.h"
#include <stdint.h>
#include <pthread.h>

/* Server configuration */
typedef struct {
    char *host;                    /* Bind address (e.g., "0.0.0.0", "127.0.0.1") */
    uint16_t port;                 /* Port to listen on */
    int max_connections;           /* Maximum concurrent connections */
    int backlog;                   /* Listen backlog queue size */
} tcp_server_config_t;

/* Server state */
typedef struct {
    tcp_server_config_t config;
    int listen_fd;                 /* Listening socket */
    volatile int running;          /* Server running flag */
    pthread_t accept_thread;       /* Accept thread */
    hl7sql_queue_manager_t *queue_mgr;  /* Queue manager */

    /* Connection tracking */
    pthread_mutex_t conn_lock;
    int active_connections;
} tcp_server_t;

/**
 * Create and initialize TCP server
 *
 * Args:
 *   config: Server configuration
 *   queue_mgr: Queue manager for query execution
 *
 * Returns:
 *   Pointer to server instance, or NULL on failure
 */
tcp_server_t *tcp_server_create(const tcp_server_config_t *config,
                                 hl7sql_queue_manager_t *queue_mgr);

/**
 * Start the TCP server
 *
 * Binds to the configured port and starts accepting connections.
 *
 * Args:
 *   server: Server instance
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int tcp_server_start(tcp_server_t *server);

/**
 * Stop the TCP server
 *
 * Stops accepting new connections and closes existing ones.
 *
 * Args:
 *   server: Server instance
 */
void tcp_server_stop(tcp_server_t *server);

/**
 * Wait for server to finish (blocking)
 *
 * Args:
 *   server: Server instance
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int tcp_server_wait(tcp_server_t *server);

/**
 * Destroy server and free resources
 *
 * Args:
 *   server: Server instance (may be NULL)
 */
void tcp_server_destroy(tcp_server_t *server);

/**
 * Get number of active connections
 *
 * Args:
 *   server: Server instance
 *
 * Returns:
 *   Number of active connections
 */
int tcp_server_get_active_connections(tcp_server_t *server);

#endif /* HL7DB_TCP_SERVER_H */
