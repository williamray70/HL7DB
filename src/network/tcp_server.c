/**
 * tcp_server.c - TCP Server Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include "network/tcp_server.h"
#include "network/protocol.h"
#include "query/hl7sql_parser.h"
#include "query/hl7sql_result.h"
#include "util/logger.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

/* Client connection context */
typedef struct {
    int fd;
    struct sockaddr_in addr;
    tcp_server_t *server;
} client_context_t;

/**
 * Handle a single client connection
 */
static void *client_handler(void *arg) {
    client_context_t *ctx = (client_context_t *)arg;
    tcp_server_t *server = ctx->server;
    int client_fd = ctx->fd;
    char client_ip[INET_ADDRSTRLEN];

    /* Get client IP for logging */
    inet_ntop(AF_INET, &ctx->addr.sin_addr, client_ip, sizeof(client_ip));
    LOG_INFO("Client connected: %s:%d", client_ip, ntohs(ctx->addr.sin_port));

    /* Process requests in a loop */
    while (server->running) {
        protocol_request_t request = {0};
        protocol_response_t *response = NULL;

        /* Read request */
        int rc = protocol_read_request(client_fd, &request);
        if (rc == -2) {
            /* Connection closed by client */
            LOG_INFO("Client disconnected: %s", client_ip);
            break;
        }
        if (rc != 0) {
            LOG_ERROR("Failed to read request from %s", client_ip);
            break;
        }

        LOG_DEBUG("Query from %s: %s", client_ip, request.query);

        /* Execute query */
        hl7sql_result_t *result = hl7sql_query(server->queue_mgr, request.query);

        if (!result) {
            /* Internal error */
            response = protocol_create_error_response(
                PROTOCOL_STATUS_INTERNAL_ERROR,
                "Query execution failed");
        } else if (result->error_code != 0) {
            /* Query error */
            protocol_status_t status = (result->error_code == 1) ?
                PROTOCOL_STATUS_PARSE_ERROR : PROTOCOL_STATUS_EXEC_ERROR;

            response = protocol_create_error_response(status, result->error_message);
        } else {
            /* Success - format result as text */
            char *result_text = hl7sql_result_to_string(result);
            response = protocol_create_success_response(result_text);
            free(result_text);
        }

        /* Clean up query result */
        if (result) {
            hl7sql_result_destroy(result);
        }

        /* Send response */
        if (response) {
            if (protocol_write_response(client_fd, response) != 0) {
                LOG_ERROR("Failed to send response to %s", client_ip);
                protocol_free_response(response);
                break;
            }
            protocol_free_response(response);
        }

        /* Clean up request */
        protocol_free_request(&request);
    }

    /* Close connection */
    close(client_fd);

    /* Update connection count */
    pthread_mutex_lock(&server->conn_lock);
    server->active_connections--;
    pthread_mutex_unlock(&server->conn_lock);

    LOG_DEBUG("Connection closed: %s (active: %d)",
              client_ip, server->active_connections);

    free(ctx);
    return NULL;
}

/**
 * Accept thread - accepts incoming connections
 */
static void *accept_thread_func(void *arg) {
    tcp_server_t *server = (tcp_server_t *)arg;

    LOG_INFO("Accept thread started");

    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        /* Accept new connection */
        int client_fd = accept(server->listen_fd,
                               (struct sockaddr *)&client_addr,
                               &addr_len);

        if (client_fd < 0) {
            if (!server->running) {
                break;  /* Server shutting down */
            }
            if (errno == EINTR) {
                continue;  /* Interrupted by signal */
            }
            LOG_ERROR("accept() failed: %s", strerror(errno));
            continue;
        }

        /* Check connection limit */
        pthread_mutex_lock(&server->conn_lock);
        if (server->active_connections >= server->config.max_connections) {
            pthread_mutex_unlock(&server->conn_lock);
            LOG_WARN("Connection limit reached, rejecting client");
            close(client_fd);
            continue;
        }
        server->active_connections++;
        pthread_mutex_unlock(&server->conn_lock);

        /* Create client context */
        client_context_t *ctx = malloc(sizeof(client_context_t));
        if (!ctx) {
            LOG_ERROR("Failed to allocate client context");
            close(client_fd);
            pthread_mutex_lock(&server->conn_lock);
            server->active_connections--;
            pthread_mutex_unlock(&server->conn_lock);
            continue;
        }

        ctx->fd = client_fd;
        ctx->addr = client_addr;
        ctx->server = server;

        /* Spawn handler thread */
        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, ctx) != 0) {
            LOG_ERROR("Failed to create client thread");
            close(client_fd);
            free(ctx);
            pthread_mutex_lock(&server->conn_lock);
            server->active_connections--;
            pthread_mutex_unlock(&server->conn_lock);
            continue;
        }

        /* Detach thread so it cleans up automatically */
        pthread_detach(thread);
    }

    LOG_INFO("Accept thread stopped");
    return NULL;
}

tcp_server_t *tcp_server_create(const tcp_server_config_t *config,
                                 hl7sql_queue_manager_t *queue_mgr) {
    if (!config || !queue_mgr) {
        LOG_ERROR("Invalid parameters to tcp_server_create");
        return NULL;
    }

    tcp_server_t *server = calloc(1, sizeof(tcp_server_t));
    if (!server) {
        LOG_ERROR("Failed to allocate server");
        return NULL;
    }

    /* Copy configuration */
    server->config = *config;
    if (config->host) {
        server->config.host = strdup(config->host);
    }

    server->queue_mgr = queue_mgr;
    server->listen_fd = -1;
    server->running = 0;
    server->active_connections = 0;

    /* Initialize mutex */
    if (pthread_mutex_init(&server->conn_lock, NULL) != 0) {
        LOG_ERROR("Failed to initialize connection mutex");
        free(server->config.host);
        free(server);
        return NULL;
    }

    LOG_INFO("TCP server created: %s:%d (max connections: %d)",
             server->config.host ? server->config.host : "0.0.0.0",
             server->config.port,
             server->config.max_connections);

    return server;
}

int tcp_server_start(tcp_server_t *server) {
    if (!server || server->running) {
        return -1;
    }

    /* Create listening socket */
    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_fd < 0) {
        LOG_ERROR("socket() failed: %s", strerror(errno));
        return -1;
    }

    /* Set socket options */
    int opt = 1;
    if (setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        LOG_WARN("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
    }

    /* Bind to address */
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server->config.port);

    if (server->config.host && strcmp(server->config.host, "0.0.0.0") != 0) {
        if (inet_pton(AF_INET, server->config.host, &addr.sin_addr) <= 0) {
            LOG_ERROR("Invalid bind address: %s", server->config.host);
            close(server->listen_fd);
            return -1;
        }
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(server->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("bind() failed on port %d: %s",
                  server->config.port, strerror(errno));
        close(server->listen_fd);
        return -1;
    }

    /* Start listening */
    int backlog = server->config.backlog > 0 ? server->config.backlog : 128;
    if (listen(server->listen_fd, backlog) < 0) {
        LOG_ERROR("listen() failed: %s", strerror(errno));
        close(server->listen_fd);
        return -1;
    }

    LOG_INFO("Listening on %s:%d",
             server->config.host ? server->config.host : "0.0.0.0",
             server->config.port);

    /* Start accept thread */
    server->running = 1;
    if (pthread_create(&server->accept_thread, NULL, accept_thread_func, server) != 0) {
        LOG_ERROR("Failed to create accept thread");
        close(server->listen_fd);
        server->running = 0;
        return -1;
    }

    LOG_INFO("TCP server started successfully");
    return 0;
}

void tcp_server_stop(tcp_server_t *server) {
    if (!server || !server->running) {
        return;
    }

    LOG_INFO("Stopping TCP server...");
    server->running = 0;

    /* Close listening socket to unblock accept() */
    if (server->listen_fd >= 0) {
        close(server->listen_fd);
        server->listen_fd = -1;
    }
}

int tcp_server_wait(tcp_server_t *server) {
    if (!server) {
        return -1;
    }

    /* Wait for accept thread to finish */
    if (pthread_join(server->accept_thread, NULL) != 0) {
        LOG_ERROR("Failed to join accept thread");
        return -1;
    }

    LOG_INFO("TCP server stopped");
    return 0;
}

void tcp_server_destroy(tcp_server_t *server) {
    if (!server) {
        return;
    }

    if (server->running) {
        tcp_server_stop(server);
        tcp_server_wait(server);
    }

    pthread_mutex_destroy(&server->conn_lock);
    free(server->config.host);
    free(server);

    LOG_DEBUG("TCP server destroyed");
}

int tcp_server_get_active_connections(tcp_server_t *server) {
    if (!server) {
        return 0;
    }

    pthread_mutex_lock(&server->conn_lock);
    int count = server->active_connections;
    pthread_mutex_unlock(&server->conn_lock);

    return count;
}
