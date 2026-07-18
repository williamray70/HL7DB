/**
 * audit.h - HIPAA Audit Logging System
 *
 * HIPAA Security Rule Compliance:
 * - 45 CFR §164.312(b) - Audit Controls
 * - 45 CFR §164.308(a)(1)(ii)(D) - Information System Activity Review
 *
 * Implements:
 * - Comprehensive audit trail for all PHI access
 * - Tamper-evident logging (append-only, checksummed)
 * - Who, what, when, where, why logging
 * - Minimum 6-year retention support
 * - Failed access attempt logging
 */

#ifndef HL7DB_AUDIT_H
#define HL7DB_AUDIT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ====================================================================
 * Audit Event Types
 * ==================================================================== */

typedef enum {
    /* Authentication Events */
    AUDIT_LOGIN_SUCCESS = 1,
    AUDIT_LOGIN_FAILED,
    AUDIT_LOGOUT,
    AUDIT_SESSION_TIMEOUT,
    AUDIT_PASSWORD_CHANGED,
    AUDIT_EMERGENCY_ACCESS,

    /* User Management Events */
    AUDIT_USER_CREATED,
    AUDIT_USER_MODIFIED,
    AUDIT_USER_DELETED,
    AUDIT_USER_LOCKED,
    AUDIT_USER_UNLOCKED,

    /* Data Access Events (PHI) */
    AUDIT_PHI_READ,             /* Read PHI data */
    AUDIT_PHI_WRITE,            /* Insert/modify PHI data */
    AUDIT_PHI_DELETE,           /* Delete PHI data */
    AUDIT_PHI_EXPORT,           /* Export PHI data */

    /* Query Events */
    AUDIT_QUERY_SELECT,
    AUDIT_QUERY_INSERT,
    AUDIT_QUERY_DELETE,
    AUDIT_QUERY_FAILED,

    /* Channel Events */
    AUDIT_CHANNEL_CREATED,
    AUDIT_CHANNEL_DROPPED,
    AUDIT_CHANNEL_ACCESSED,

    /* System Events */
    AUDIT_SYSTEM_START,
    AUDIT_SYSTEM_STOP,
    AUDIT_CONFIG_CHANGED,
    AUDIT_AUDIT_LOG_ACCESSED,   /* Someone viewed audit logs */

    /* Security Events */
    AUDIT_ACCESS_DENIED,
    AUDIT_PERMISSION_DENIED,
    AUDIT_SUSPICIOUS_ACTIVITY,
    AUDIT_ACCOUNT_LOCKOUT
} audit_event_type_t;

/* ====================================================================
 * Audit Severity Levels
 * ==================================================================== */

typedef enum {
    AUDIT_SEVERITY_INFO = 0,        /* Informational */
    AUDIT_SEVERITY_WARNING,         /* Potential issue */
    AUDIT_SEVERITY_ERROR,           /* Error condition */
    AUDIT_SEVERITY_CRITICAL         /* Critical security event */
} audit_severity_t;

/* ====================================================================
 * Audit Event Record
 * ==================================================================== */

#define AUDIT_MAX_USERNAME 64
#define AUDIT_MAX_IP 64
#define AUDIT_MAX_DETAILS 512
#define AUDIT_MAX_RESOURCE 256
#define AUDIT_CHECKSUM_LEN 32

typedef struct {
    /* Core event information */
    uint64_t event_id;                  /* Unique event ID (sequential) */
    time_t timestamp;                   /* Event timestamp */
    audit_event_type_t event_type;      /* Event type */
    audit_severity_t severity;          /* Severity level */

    /* WHO: User identification */
    uint32_t user_id;                   /* User ID (0 = system/unknown) */
    char username[AUDIT_MAX_USERNAME];  /* Username */
    char session_token[65];             /* Session token (first 32 chars) */

    /* WHERE: Source information */
    char client_ip[AUDIT_MAX_IP];       /* Client IP address */
    char hostname[128];                 /* Client hostname (if available) */

    /* WHAT: Resource accessed */
    char resource[AUDIT_MAX_RESOURCE];  /* Resource (channel, user, etc.) */
    char resource_type[32];             /* Resource type */

    /* WHY/HOW: Action details */
    char action[128];                   /* Action description */
    char details[AUDIT_MAX_DETAILS];    /* Additional details */

    /* Result */
    bool success;                       /* Operation succeeded? */
    int error_code;                     /* Error code (if failed) */

    /* PHI tracking */
    bool phi_accessed;                  /* PHI was accessed? */
    uint32_t phi_record_count;          /* Number of PHI records accessed */

    /* Tamper detection */
    char checksum[AUDIT_CHECKSUM_LEN + 1]; /* SHA-256 checksum of record */
    uint64_t previous_event_id;         /* Previous event ID (chain) */
} audit_event_t;

/* ====================================================================
 * Audit System
 * ==================================================================== */

typedef struct audit_system audit_system_t;

/**
 * Initialize audit logging system
 *
 * Creates audit log file and initializes in-memory buffer.
 *
 * Args:
 *   log_file_path: Path to audit log file (NULL = default)
 *   buffer_size: Size of in-memory buffer before flush
 *
 * Returns:
 *   Pointer to audit system, or NULL on failure
 */
audit_system_t *audit_system_init(const char *log_file_path, size_t buffer_size);

/**
 * Shutdown audit system and flush pending events
 *
 * Args:
 *   sys: Audit system
 */
void audit_system_destroy(audit_system_t *sys);

/* ====================================================================
 * Audit Event Logging
 * ==================================================================== */

/**
 * Log an audit event
 *
 * This is the core audit logging function. All other audit_log_*
 * functions are convenience wrappers around this.
 *
 * Args:
 *   sys: Audit system
 *   event: Event to log
 *
 * Returns:
 *   Event ID on success, 0 on failure
 */
uint64_t audit_log_event(audit_system_t *sys, const audit_event_t *event);

/**
 * Log authentication event (login/logout)
 *
 * Args:
 *   sys: Audit system
 *   event_type: AUDIT_LOGIN_SUCCESS, AUDIT_LOGIN_FAILED, AUDIT_LOGOUT
 *   username: Username
 *   client_ip: Client IP address
 *   success: Authentication succeeded?
 *   details: Additional details (reason, error message, etc.)
 *
 * Returns:
 *   Event ID on success, 0 on failure
 */
uint64_t audit_log_auth(audit_system_t *sys,
                        audit_event_type_t event_type,
                        const char *username,
                        const char *client_ip,
                        bool success,
                        const char *details);

/**
 * Log PHI access event
 *
 * CRITICAL: Must be called for ALL PHI access (read/write/delete).
 *
 * Args:
 *   sys: Audit system
 *   event_type: AUDIT_PHI_READ, AUDIT_PHI_WRITE, AUDIT_PHI_DELETE
 *   user_id: User ID
 *   username: Username
 *   session_token: Session token
 *   resource: Resource accessed (channel name, message ID, etc.)
 *   record_count: Number of PHI records accessed
 *   client_ip: Client IP address
 *   details: Query or operation details (with PHI redacted!)
 *
 * Returns:
 *   Event ID on success, 0 on failure
 */
uint64_t audit_log_phi_access(audit_system_t *sys,
                               audit_event_type_t event_type,
                               uint32_t user_id,
                               const char *username,
                               const char *session_token,
                               const char *resource,
                               uint32_t record_count,
                               const char *client_ip,
                               const char *details);

/**
 * Log query execution
 *
 * Args:
 *   sys: Audit system
 *   user_id: User ID
 *   username: Username
 *   session_token: Session token
 *   query: Query string (with PHI redacted!)
 *   channel: Channel name (if applicable)
 *   rows_affected: Number of rows affected
 *   phi_accessed: Query accessed PHI?
 *   success: Query succeeded?
 *   client_ip: Client IP address
 *
 * Returns:
 *   Event ID on success, 0 on failure
 */
uint64_t audit_log_query(audit_system_t *sys,
                         uint32_t user_id,
                         const char *username,
                         const char *session_token,
                         const char *query,
                         const char *channel,
                         uint32_t rows_affected,
                         bool phi_accessed,
                         bool success,
                         const char *client_ip);

/**
 * Log access denied event
 *
 * Args:
 *   sys: Audit system
 *   user_id: User ID (0 if unauthenticated)
 *   username: Username
 *   session_token: Session token (NULL if none)
 *   resource: Resource attempted to access
 *   action: Action attempted
 *   reason: Denial reason
 *   client_ip: Client IP address
 *
 * Returns:
 *   Event ID on success, 0 on failure
 */
uint64_t audit_log_access_denied(audit_system_t *sys,
                                  uint32_t user_id,
                                  const char *username,
                                  const char *session_token,
                                  const char *resource,
                                  const char *action,
                                  const char *reason,
                                  const char *client_ip);

/**
 * Log emergency access event
 *
 * Enhanced logging for break-glass emergency access.
 *
 * Args:
 *   sys: Audit system
 *   user_id: User ID
 *   username: Username
 *   session_token: Session token
 *   reason: Emergency access reason (required)
 *   client_ip: Client IP address
 *
 * Returns:
 *   Event ID on success, 0 on failure
 */
uint64_t audit_log_emergency_access(audit_system_t *sys,
                                     uint32_t user_id,
                                     const char *username,
                                     const char *session_token,
                                     const char *reason,
                                     const char *client_ip);

/**
 * Log system event
 *
 * Args:
 *   sys: Audit system
 *   event_type: Event type (AUDIT_SYSTEM_START, etc.)
 *   details: Event details
 *
 * Returns:
 *   Event ID on success, 0 on failure
 */
uint64_t audit_log_system(audit_system_t *sys,
                          audit_event_type_t event_type,
                          const char *details);

/* ====================================================================
 * Audit Log Retrieval & Analysis
 * ==================================================================== */

/**
 * Query filter for audit log retrieval
 */
typedef struct {
    /* Time range */
    time_t start_time;          /* 0 = no start limit */
    time_t end_time;            /* 0 = no end limit */

    /* User filter */
    uint32_t user_id;           /* 0 = any user */
    char username[AUDIT_MAX_USERNAME]; /* Empty = any username */

    /* Event type filter */
    audit_event_type_t event_type; /* 0 = any event type */

    /* Severity filter */
    audit_severity_t min_severity; /* Minimum severity */

    /* PHI access filter */
    bool phi_only;              /* Only show PHI access events */

    /* Resource filter */
    char resource[AUDIT_MAX_RESOURCE]; /* Empty = any resource */

    /* Result filter */
    int success_filter;         /* -1 = any, 0 = failed only, 1 = success only */

    /* Limit */
    uint32_t max_results;       /* 0 = unlimited */
} audit_query_filter_t;

/**
 * Query audit log
 *
 * Requires PERM_AUDIT_VIEW permission.
 * This function itself generates an audit log entry.
 *
 * Args:
 *   sys: Audit system
 *   filter: Query filter
 *   results_out: Output array of events (caller must free)
 *   count_out: Number of results returned
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int audit_query_log(audit_system_t *sys,
                    const audit_query_filter_t *filter,
                    audit_event_t **results_out,
                    size_t *count_out);

/**
 * Get audit statistics
 *
 * Args:
 *   sys: Audit system
 *   start_time: Start of time range (0 = all time)
 *   end_time: End of time range (0 = now)
 *   total_events_out: Total events in range
 *   phi_access_count_out: PHI access events
 *   failed_access_count_out: Failed access attempts
 *   unique_users_out: Number of unique users
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int audit_get_statistics(audit_system_t *sys,
                         time_t start_time,
                         time_t end_time,
                         uint64_t *total_events_out,
                         uint64_t *phi_access_count_out,
                         uint64_t *failed_access_count_out,
                         uint32_t *unique_users_out);

/* ====================================================================
 * Audit Log Integrity
 * ==================================================================== */

/**
 * Verify audit log integrity
 *
 * Checks that audit log has not been tampered with by validating
 * checksums and event chain.
 *
 * Args:
 *   sys: Audit system
 *   start_event_id: Start of range to verify (0 = beginning)
 *   end_event_id: End of range to verify (0 = current)
 *
 * Returns:
 *   Number of integrity violations found (0 = intact)
 */
int audit_verify_integrity(audit_system_t *sys,
                           uint64_t start_event_id,
                           uint64_t end_event_id);

/**
 * Export audit log for archival
 *
 * Exports audit log in a tamper-evident format suitable for
 * long-term archival (HIPAA requires 6 year retention).
 *
 * Args:
 *   sys: Audit system
 *   output_path: Path to export file
 *   start_time: Start of time range (0 = all)
 *   end_time: End of time range (0 = now)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int audit_export_log(audit_system_t *sys,
                     const char *output_path,
                     time_t start_time,
                     time_t end_time);

/* ====================================================================
 * Utility Functions
 * ==================================================================== */

/**
 * Get event type name as string
 *
 * Args:
 *   event_type: Event type
 *
 * Returns:
 *   Event type name string
 */
const char *audit_event_type_to_string(audit_event_type_t event_type);

/**
 * Get severity name as string
 *
 * Args:
 *   severity: Severity level
 *
 * Returns:
 *   Severity name string
 */
const char *audit_severity_to_string(audit_severity_t severity);

/**
 * Create default query filter (all events)
 *
 * Returns:
 *   Default filter
 */
audit_query_filter_t audit_create_default_filter(void);

#endif /* HL7DB_AUDIT_H */
