/**
 * tls_server.c - TLS/SSL Secure TCP Server Implementation
 *
 * HIPAA Compliance: 45 CFR §164.312(e)(1) - Transmission Security
 *
 * Implements TLS 1.2+ encryption for all network communications
 * to protect PHI data in transit.
 */

#define _POSIX_C_SOURCE 200809L

#include "network/tls_server.h"
#include "util/logger.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

/* TLS Server Structure */
struct tls_server {
    tls_config_t config;
    hl7sql_channel_manager_t *queue_mgr;

    SSL_CTX *ssl_ctx;
    int server_fd;
    bool running;

    pthread_t server_thread;
    pthread_mutex_t lock;

    /* Statistics */
    uint64_t total_connections;
    uint64_t active_connections;
    uint64_t failed_handshakes;
    uint64_t queries_processed;
    uint64_t queries_failed;
};

/* ====================================================================
 * TLS/SSL Context Creation
 * ==================================================================== */

static SSL_CTX *create_ssl_context(const tls_config_t *config) {
    const SSL_METHOD *method;

    /* Use TLS 1.2+ only */
    method = TLS_server_method();

    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        LOG_ERROR("Failed to create SSL context");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    /* Set minimum TLS version to 1.2 */
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    /* Set strong cipher suites (HIPAA compliant) */
    const char *cipher_list =
        "ECDHE-RSA-AES256-GCM-SHA384:"
        "ECDHE-RSA-AES128-GCM-SHA256:"
        "DHE-RSA-AES256-GCM-SHA384:"
        "DHE-RSA-AES128-GCM-SHA256";

    if (SSL_CTX_set_cipher_list(ctx, cipher_list) != 1) {
        LOG_ERROR("Failed to set cipher list");
        SSL_CTX_free(ctx);
        return NULL;
    }

    /* Load server certificate */
    if (SSL_CTX_use_certificate_file(ctx, config->cert_file, SSL_FILETYPE_PEM) <= 0) {
        LOG_ERROR("Failed to load server certificate from %s", config->cert_file);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    /* Load server private key */
    if (SSL_CTX_use_PrivateKey_file(ctx, config->key_file, SSL_FILETYPE_PEM) <= 0) {
        LOG_ERROR("Failed to load server private key from %s", config->key_file);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    /* Verify private key */
    if (!SSL_CTX_check_private_key(ctx)) {
        LOG_ERROR("Private key does not match the certificate");
        SSL_CTX_free(ctx);
        return NULL;
    }

    /* Optional: Load CA certificate for client verification */
    if (config->verify_client_cert && config->ca_file[0] != '\0') {
        if (SSL_CTX_load_verify_locations(ctx, config->ca_file, NULL) != 1) {
            LOG_ERROR("Failed to load CA certificate from %s", config->ca_file);
            SSL_CTX_free(ctx);
            return NULL;
        }

        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    }

    LOG_INFO("SSL context created successfully");
    return ctx;
}

/* ====================================================================
 * TLS Server Lifecycle
 * ==================================================================== */

tls_server_t *tls_server_create(const tls_config_t *config,
                                 hl7sql_channel_manager_t *queue_mgr) {
    if (!config || !queue_mgr) {
        return NULL;
    }

    tls_server_t *server = calloc(1, sizeof(tls_server_t));
    if (!server) {
        LOG_ERROR("Failed to allocate TLS server");
        return NULL;
    }

    server->config = *config;
    server->queue_mgr = queue_mgr;
    server->server_fd = -1;

    /* Initialize OpenSSL */
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    /* Create SSL context */
    server->ssl_ctx = create_ssl_context(config);
    if (!server->ssl_ctx) {
        free(server);
        return NULL;
    }

    /* Initialize mutex */
    if (pthread_mutex_init(&server->lock, NULL) != 0) {
        LOG_ERROR("Failed to initialize mutex");
        SSL_CTX_free(server->ssl_ctx);
        free(server);
        return NULL;
    }

    LOG_INFO("TLS server created (port=%d, cert=%s)",
             config->port, config->cert_file);

    return server;
}

void tls_server_destroy(tls_server_t *server) {
    if (!server) {
        return;
    }

    /* Stop server if running */
    if (server->running) {
        tls_server_stop(server);
    }

    /* Cleanup SSL context */
    if (server->ssl_ctx) {
        SSL_CTX_free(server->ssl_ctx);
    }

    /* Cleanup mutex */
    pthread_mutex_destroy(&server->lock);

    free(server);
    LOG_INFO("TLS server destroyed");
}

bool tls_server_is_running(const tls_server_t *server) {
    if (!server) {
        return false;
    }
    return server->running;
}

/* ====================================================================
 * Client Connection Handler
 * ==================================================================== */

typedef struct {
    SSL *ssl;
    int fd;
    struct sockaddr_in addr;
    tls_server_t *server;
} tls_client_context_t;

static void *tls_client_handler(void *arg) {
    tls_client_context_t *ctx = (tls_client_context_t *)arg;
    tls_server_t *server = ctx->server;
    SSL *ssl = ctx->ssl;
    char client_ip[INET_ADDRSTRLEN];

    /* Get client IP */
    inet_ntop(AF_INET, &ctx->addr.sin_addr, client_ip, sizeof(client_ip));

    pthread_mutex_lock(&server->lock);
    server->active_connections++;
    pthread_mutex_unlock(&server->lock);

    LOG_INFO("TLS client connected: %s:%d", client_ip, ntohs(ctx->addr.sin_port));

    /* Perform SSL handshake */
    if (SSL_accept(ssl) <= 0) {
        LOG_ERROR("TLS handshake failed for %s", client_ip);
        ERR_print_errors_fp(stderr);
        pthread_mutex_lock(&server->lock);
        server->failed_handshakes++;
        server->active_connections--;
        pthread_mutex_unlock(&server->lock);
        SSL_free(ssl);
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    LOG_INFO("TLS handshake successful with %s", client_ip);

    /* Process requests - simplified version without full protocol parsing */
    char buffer[4096];
    while (server->running) {
        int bytes = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) {
            int err = SSL_get_error(ssl, bytes);
            if (err == SSL_ERROR_ZERO_RETURN) {
                LOG_INFO("TLS client disconnected: %s", client_ip);
            } else {
                LOG_ERROR("TLS read error from %s: %d", client_ip, err);
            }
            break;
        }

        buffer[bytes] = '\0';
        LOG_DEBUG("TLS query from %s: %s", client_ip, buffer);

        /* Simple echo response for now - in production would execute query */
        const char *response = "OK\n";
        if (SSL_write(ssl, response, strlen(response)) <= 0) {
            LOG_ERROR("TLS write error to %s", client_ip);
            break;
        }

        pthread_mutex_lock(&server->lock);
        server->queries_processed++;
        pthread_mutex_unlock(&server->lock);
    }

    /* Cleanup */
    pthread_mutex_lock(&server->lock);
    server->active_connections--;
    pthread_mutex_unlock(&server->lock);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(ctx->fd);
    free(ctx);

    LOG_INFO("TLS client handler finished: %s", client_ip);
    return NULL;
}

/* ====================================================================
 * Server Thread
 * ==================================================================== */

static void *server_thread_func(void *arg) {
    tls_server_t *server = (tls_server_t *)arg;

    LOG_INFO("TLS server thread started");

    /* Create listening socket */
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        LOG_ERROR("Failed to create TLS socket: %s", strerror(errno));
        return NULL;
    }

    /* Set socket options */
    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_WARN("Failed to set SO_REUSEADDR: %s", strerror(errno));
    }

    /* Bind to port */
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->config.port);

    if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind TLS server to port %d: %s",
                  server->config.port, strerror(errno));
        close(server->server_fd);
        return NULL;
    }

    /* Listen for connections */
    if (listen(server->server_fd, 128) < 0) {
        LOG_ERROR("Failed to listen on TLS socket: %s", strerror(errno));
        close(server->server_fd);
        return NULL;
    }

    LOG_INFO("TLS server listening on port %d", server->config.port);

    /* Accept loop */
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server->server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (server->running) {
                LOG_ERROR("Accept failed: %s", strerror(errno));
            }
            continue;
        }

        pthread_mutex_lock(&server->lock);
        server->total_connections++;
        pthread_mutex_unlock(&server->lock);

        /* Create SSL connection */
        SSL *ssl = SSL_new(server->ssl_ctx);
        if (!ssl) {
            LOG_ERROR("Failed to create SSL connection");
            close(client_fd);
            continue;
        }

        SSL_set_fd(ssl, client_fd);

        /* Create client context */
        tls_client_context_t *ctx = malloc(sizeof(tls_client_context_t));
        if (!ctx) {
            LOG_ERROR("Failed to allocate client context");
            SSL_free(ssl);
            close(client_fd);
            continue;
        }

        ctx->ssl = ssl;
        ctx->fd = client_fd;
        ctx->addr = client_addr;
        ctx->server = server;

        /* Handle client in new thread */
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, tls_client_handler, ctx) != 0) {
            LOG_ERROR("Failed to create client thread: %s", strerror(errno));
            SSL_free(ssl);
            close(client_fd);
            free(ctx);
            continue;
        }

        /* Detach thread - it will clean up itself */
        pthread_detach(client_thread);
    }

    LOG_INFO("TLS server thread stopped");
    return NULL;
}

int tls_server_start(tls_server_t *server) {
    if (!server) {
        return -1;
    }

    if (server->running) {
        LOG_WARN("TLS server already running");
        return -1;
    }

    server->running = true;

    /* Create server thread */
    if (pthread_create(&server->server_thread, NULL, server_thread_func, server) != 0) {
        LOG_ERROR("Failed to create server thread: %s", strerror(errno));
        server->running = false;
        return -1;
    }

    LOG_INFO("TLS server started on port %d", server->config.port);
    return 0;
}

int tls_server_stop(tls_server_t *server) {
    if (!server || !server->running) {
        return -1;
    }

    server->running = false;

    /* Wait for server thread to finish */
    pthread_join(server->server_thread, NULL);

    /* Close server socket */
    if (server->server_fd >= 0) {
        close(server->server_fd);
        server->server_fd = -1;
    }

    LOG_INFO("TLS server stopped");
    return 0;
}

/* ====================================================================
 * Statistics
 * ==================================================================== */

int tls_server_get_stats(const tls_server_t *server, tls_server_stats_t *stats_out) {
    if (!server || !stats_out) {
        return -1;
    }

    pthread_mutex_lock((pthread_mutex_t *)&server->lock);

    stats_out->total_connections = server->total_connections;
    stats_out->active_connections = server->active_connections;
    stats_out->failed_handshakes = server->failed_handshakes;
    stats_out->queries_processed = server->queries_processed;
    stats_out->queries_failed = server->queries_failed;

    pthread_mutex_unlock((pthread_mutex_t *)&server->lock);

    return 0;
}

/* ====================================================================
 * Default Configuration
 * ==================================================================== */

tls_config_t tls_get_default_config(void) {
    tls_config_t config = {
        .port = 7778,
        .max_connections = 100,
        .cert_file = "/etc/hl7db/server-cert.pem",
        .key_file = "/etc/hl7db/server-key.pem",
        .ca_file = "",
        .require_client_cert = false,
        .verify_client_cert = false,
        .tls_min_version = "TLS1.2",
        .cipher_list = NULL
    };
    return config;
}
