/**
 * tls_server.h - TLS/SSL Secure TCP Server
 *
 * HIPAA Compliance: 45 CFR §164.312(e)(1) - Transmission Security
 *
 * Implements TLS 1.2+ encryption for all network communications
 * to protect PHI data in transit.
 */

#ifndef HL7DB_TLS_SERVER_H
#define HL7DB_TLS_SERVER_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct tls_server tls_server_t;
typedef struct query_queue_manager hl7sql_channel_manager_t;

/* ====================================================================
 * TLS Configuration
 * ==================================================================== */

/**
 * TLS server configuration
 */
typedef struct {
    uint16_t port;                  /* Port to listen on */
    int max_connections;            /* Maximum concurrent connections */

    char cert_file[256];            /* Server certificate file (PEM) */
    char key_file[256];             /* Server private key file (PEM) */
    char ca_file[256];              /* CA certificate for client verification (optional) */

    bool require_client_cert;       /* Require client certificate authentication */
    bool verify_client_cert;        /* Verify client certificates against CA */

    const char *tls_min_version;    /* Minimum TLS version (TLS1.2, TLS1.3) */
    const char *cipher_list;        /* Allowed cipher suites (NULL = secure defaults) */
} tls_config_t;

/**
 * Get default TLS configuration (HIPAA-compliant secure defaults)
 *
 * Returns:
 *   Default configuration with:
 *   - TLS 1.2+ only
 *   - Strong cipher suites
 *   - Client certificate verification enabled
 */
tls_config_t tls_get_default_config(void);

/* ====================================================================
 * TLS Server Lifecycle
 * ==================================================================== */

/**
 * Create and initialize TLS server
 *
 * Args:
 *   config: TLS configuration
 *   queue_mgr: HL7SQL channel manager for query processing
 *
 * Returns:
 *   TLS server instance, or NULL on failure
 */
tls_server_t *tls_server_create(const tls_config_t *config,
                                 hl7sql_channel_manager_t *queue_mgr);

/**
 * Start TLS server (begins accepting connections)
 *
 * This function starts the server in a background thread.
 *
 * Args:
 *   server: TLS server instance
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int tls_server_start(tls_server_t *server);

/**
 * Stop TLS server (graceful shutdown)
 *
 * Args:
 *   server: TLS server instance
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int tls_server_stop(tls_server_t *server);

/**
 * Destroy TLS server and free resources
 *
 * Args:
 *   server: TLS server instance
 */
void tls_server_destroy(tls_server_t *server);

/**
 * Check if TLS server is running
 *
 * Args:
 *   server: TLS server instance
 *
 * Returns:
 *   true if running, false otherwise
 */
bool tls_server_is_running(const tls_server_t *server);

/* ====================================================================
 * Server Statistics
 * ==================================================================== */

/**
 * TLS server statistics
 */
typedef struct {
    uint64_t total_connections;     /* Total connections accepted */
    uint64_t active_connections;    /* Currently active connections */
    uint64_t failed_handshakes;     /* Failed TLS handshakes */
    uint64_t queries_processed;     /* Total queries processed */
    uint64_t queries_failed;        /* Failed queries */
} tls_server_stats_t;

/**
 * Get server statistics
 *
 * Args:
 *   server: TLS server instance
 *   stats: Output statistics structure
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int tls_server_get_stats(const tls_server_t *server, tls_server_stats_t *stats);

/* ====================================================================
 * Certificate Generation Utilities
 * ==================================================================== */

/**
 * Generate self-signed certificate for testing
 *
 * WARNING: Self-signed certificates should NOT be used in production!
 * For production, obtain certificates from a trusted CA.
 *
 * Args:
 *   cert_file: Output path for certificate (PEM format)
 *   key_file: Output path for private key (PEM format)
 *   common_name: Common Name for certificate (e.g., "hl7db.example.com")
 *   days_valid: Number of days certificate is valid
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int tls_generate_self_signed_cert(const char *cert_file,
                                    const char *key_file,
                                    const char *common_name,
                                    int days_valid);

#endif /* HL7DB_TLS_SERVER_H */
