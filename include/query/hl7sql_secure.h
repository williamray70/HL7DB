/**
 * hl7sql_secure.h - Secure Query Execution with Auth & Audit
 *
 * This module wraps the HL7SQL query engine with authentication,
 * authorization, and audit logging to ensure HIPAA compliance.
 *
 * All queries MUST go through this interface in production to ensure:
 * - User authentication (valid session)
 * - Permission checks (RBAC)
 * - Audit logging (all access tracked)
 * - PHI redaction (safe output)
 */

#ifndef HL7DB_HL7SQL_SECURE_H
#define HL7DB_HL7SQL_SECURE_H

#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_result.h"
#include "security/auth.h"
#include "security/audit.h"

/* ====================================================================
 * Secure Query Context
 * ==================================================================== */

typedef struct {
    hl7sql_queue_manager_t *queue_mgr;  /* Channel manager */
    auth_system_t *auth;                     /* Authentication system */
    audit_system_t *audit;                   /* Audit logging (shared with auth) */
} hl7sql_secure_context_t;

/**
 * Create secure query context
 *
 * Initializes authentication, authorization, and audit logging systems.
 *
 * Returns:
 *   Secure context, or NULL on failure
 */
hl7sql_secure_context_t *hl7sql_secure_context_create(void);

/**
 * Destroy secure query context
 *
 * Shuts down all systems and frees resources.
 *
 * Args:
 *   ctx: Secure context
 */
void hl7sql_secure_context_destroy(hl7sql_secure_context_t *ctx);

/* ====================================================================
 * Secure Query Execution
 * ==================================================================== */

/**
 * Execute HL7SQL query with authentication and authorization
 *
 * This is the main entry point for all query execution.
 * Performs the following security checks:
 *   1. Validate session token
 *   2. Check permissions based on query type
 *   3. Execute query if authorized
 *   4. Audit log the query execution
 *   5. Audit log PHI access if applicable
 *
 * Permission requirements by query type:
 *   - SELECT: PERM_QUERY_SELECT + PERM_READ_PHI (if returns data)
 *   - INSERT: PERM_QUERY_INSERT + PERM_WRITE_PHI
 *   - DELETE: PERM_QUERY_DELETE + PERM_DELETE_PHI
 *   - CREATE CHANNEL: PERM_CHANNEL_CREATE
 *   - DROP CHANNEL: PERM_CHANNEL_DROP
 *   - POP MESSAGE: PERM_QUERY_SELECT + PERM_READ_PHI
 *
 * Args:
 *   ctx: Secure context
 *   session_token: User's session token
 *   query: HL7SQL query string
 *   client_ip: Client IP address (for audit)
 *
 * Returns:
 *   Query result (caller must free), or error result if unauthorized
 */
hl7sql_result_t *hl7sql_secure_query(hl7sql_secure_context_t *ctx,
                                      const char *session_token,
                                      const char *query,
                                      const char *client_ip);

/**
 * Execute query with PHI redaction in result
 *
 * Same as hl7sql_secure_query, but automatically redacts PHI in the
 * returned result set. Safe for displaying to users or logging.
 *
 * Args:
 *   ctx: Secure context
 *   session_token: User's session token
 *   query: HL7SQL query string
 *   client_ip: Client IP address
 *   redaction_config: PHI redaction config (NULL = use default)
 *
 * Returns:
 *   Query result with redacted PHI (caller must free)
 */
hl7sql_result_t *hl7sql_secure_query_redacted(
    hl7sql_secure_context_t *ctx,
    const char *session_token,
    const char *query,
    const char *client_ip,
    const void *redaction_config);

/* ====================================================================
 * Authentication Helpers
 * ==================================================================== */

/**
 * Login user and create session
 *
 * Convenience wrapper around auth_login with audit logging.
 *
 * Args:
 *   ctx: Secure context
 *   username: Username
 *   password: Password
 *   client_ip: Client IP address
 *   client_info: Client information
 *
 * Returns:
 *   Session token (caller must free), or NULL on failure
 */
char *hl7sql_secure_login(hl7sql_secure_context_t *ctx,
                          const char *username,
                          const char *password,
                          const char *client_ip,
                          const char *client_info);

/**
 * Logout user and destroy session
 *
 * Args:
 *   ctx: Secure context
 *   session_token: Session token to invalidate
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int hl7sql_secure_logout(hl7sql_secure_context_t *ctx,
                         const char *session_token);

/**
 * Emergency access login (break-glass)
 *
 * Args:
 *   ctx: Secure context
 *   username: Username
 *   password: Password
 *   reason: Emergency access reason (required)
 *   client_ip: Client IP address
 *   client_info: Client information
 *
 * Returns:
 *   Session token (caller must free), or NULL on failure
 */
char *hl7sql_secure_emergency_login(hl7sql_secure_context_t *ctx,
                                     const char *username,
                                     const char *password,
                                     const char *reason,
                                     const char *client_ip,
                                     const char *client_info);

/* ====================================================================
 * User Management (Admin Functions)
 * ==================================================================== */

/**
 * Create new user (requires PERM_USER_MANAGE)
 *
 * Args:
 *   ctx: Secure context
 *   admin_session: Admin session token
 *   username: New username
 *   password: Initial password
 *   full_name: User's full name
 *   role: User's role
 *
 * Returns:
 *   User ID on success, 0 on failure
 */
uint32_t hl7sql_secure_create_user(hl7sql_secure_context_t *ctx,
                                    const char *admin_session,
                                    const char *username,
                                    const char *password,
                                    const char *full_name,
                                    role_type_t role);

/**
 * Change password
 *
 * Args:
 *   ctx: Secure context
 *   session_token: User's session token
 *   old_password: Current password
 *   new_password: New password
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int hl7sql_secure_change_password(hl7sql_secure_context_t *ctx,
                                   const char *session_token,
                                   const char *old_password,
                                   const char *new_password);

/* ====================================================================
 * Utility Functions
 * ==================================================================== */

/**
 * Get current user info from session
 *
 * Args:
 *   ctx: Secure context
 *   session_token: Session token
 *
 * Returns:
 *   Session info (do not free), or NULL if invalid
 */
const auth_session_t *hl7sql_secure_get_session_info(
    hl7sql_secure_context_t *ctx,
    const char *session_token);

/**
 * Check if session has specific permission
 *
 * Args:
 *   ctx: Secure context
 *   session_token: Session token
 *   permission: Permission to check
 *
 * Returns:
 *   true if authorized, false otherwise
 */
bool hl7sql_secure_has_permission(hl7sql_secure_context_t *ctx,
                                   const char *session_token,
                                   permission_t permission);

#endif /* HL7DB_HL7SQL_SECURE_H */
