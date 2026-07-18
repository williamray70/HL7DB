/**
 * security_monitor.h - Security Monitoring and Alerting
 *
 * HIPAA Compliance Enhancement:
 * - Real-time security event monitoring
 * - Anomaly detection for suspicious activities
 * - Automated alerting for security incidents
 * - Compliance with §164.308(a)(1)(ii)(D) - Information System Activity Review
 */

#ifndef HL7DB_SECURITY_MONITOR_H
#define HL7DB_SECURITY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ====================================================================
 * Security Event Types
 * ==================================================================== */

typedef enum {
    SECURITY_EVENT_BRUTE_FORCE,         /* Brute force attack detected */
    SECURITY_EVENT_UNUSUAL_ACCESS,      /* Unusual PHI access pattern */
    SECURITY_EVENT_PRIVILEGE_ESCALATION,/* Privilege escalation attempt */
    SECURITY_EVENT_DATA_EXFILTRATION,   /* Large data export */
    SECURITY_EVENT_AFTER_HOURS_ACCESS,  /* Access outside business hours */
    SECURITY_EVENT_MULTIPLE_FAILURES,   /* Multiple failed operations */
    SECURITY_EVENT_ACCOUNT_LOCKED,      /* Account locked due to failures */
    SECURITY_EVENT_EMERGENCY_ACCESS,    /* Emergency break-glass access used */
    SECURITY_EVENT_CONFIG_CHANGE,       /* System configuration changed */
    SECURITY_EVENT_AUDIT_LOG_ACCESS,    /* Audit log accessed */
    SECURITY_EVENT_SUSPICIOUS_QUERY,    /* Suspicious database query */
    SECURITY_EVENT_TLS_FAILURE,         /* TLS handshake failure */
    SECURITY_EVENT_CERT_EXPIRING        /* Certificate expiring soon */
} security_event_type_t;

/**
 * Security event severity
 */
typedef enum {
    SECURITY_SEVERITY_LOW,
    SECURITY_SEVERITY_MEDIUM,
    SECURITY_SEVERITY_HIGH,
    SECURITY_SEVERITY_CRITICAL
} security_severity_t;

/**
 * Security event
 */
typedef struct {
    security_event_type_t type;     /* Event type */
    security_severity_t severity;   /* Event severity */
    time_t timestamp;               /* When event occurred */
    uint32_t user_id;               /* User ID (if applicable) */
    char username[64];              /* Username */
    char client_ip[64];             /* Client IP address */
    char description[256];          /* Event description */
    char details[512];              /* Additional details */
} security_event_t;

/* ====================================================================
 * Alert Configuration
 * ==================================================================== */

/**
 * Alert action types
 */
typedef enum {
    ALERT_ACTION_LOG,               /* Log to file */
    ALERT_ACTION_EMAIL,             /* Send email */
    ALERT_ACTION_SYSLOG,            /* Send to syslog */
    ALERT_ACTION_WEBHOOK,           /* HTTP POST to webhook */
    ALERT_ACTION_LOCK_ACCOUNT,      /* Lock user account */
    ALERT_ACTION_BLOCK_IP           /* Block IP address */
} alert_action_t;

/**
 * Alert rule
 */
typedef struct {
    security_event_type_t event_type;   /* Event type to match */
    security_severity_t min_severity;   /* Minimum severity */
    alert_action_t action;              /* Action to take */
    char config[256];                   /* Action configuration (email, URL, etc.) */
    bool enabled;                       /* Rule enabled? */
} alert_rule_t;

/* ====================================================================
 * Security Monitor System
 * ==================================================================== */

typedef struct security_monitor security_monitor_t;

/**
 * Initialize security monitor
 *
 * Returns:
 *   Security monitor instance, or NULL on failure
 */
security_monitor_t *security_monitor_init(void);

/**
 * Destroy security monitor
 *
 * Args:
 *   monitor: Security monitor instance
 */
void security_monitor_destroy(security_monitor_t *monitor);

/* ====================================================================
 * Event Reporting
 * ==================================================================== */

/**
 * Report security event
 *
 * Args:
 *   monitor: Security monitor
 *   event: Security event to report
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_report_event(security_monitor_t *monitor,
                                   const security_event_t *event);

/**
 * Report brute force attack
 *
 * Convenience function for common security event.
 *
 * Args:
 *   monitor: Security monitor
 *   username: Username being targeted
 *   client_ip: Attacker IP address
 *   attempt_count: Number of failed attempts
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_report_brute_force(security_monitor_t *monitor,
                                        const char *username,
                                        const char *client_ip,
                                        int attempt_count);

/* ====================================================================
 * Anomaly Detection
 * ==================================================================== */

/**
 * Anomaly detection configuration
 */
typedef struct {
    bool detect_unusual_hours;          /* Detect access outside 8am-6pm */
    bool detect_unusual_volume;         /* Detect unusual query volume */
    bool detect_unusual_patterns;       /* Detect unusual access patterns */
    int baseline_days;                  /* Days to establish baseline */
    float threshold_multiplier;         /* Threshold for anomaly (2.0 = 2x normal) */
} anomaly_config_t;

/**
 * Configure anomaly detection
 *
 * Args:
 *   monitor: Security monitor
 *   config: Anomaly detection configuration
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_configure_anomaly_detection(security_monitor_t *monitor,
                                                  const anomaly_config_t *config);

/**
 * Analyze user activity for anomalies
 *
 * Args:
 *   monitor: Security monitor
 *   user_id: User ID to analyze
 *   anomaly_detected_out: Output flag for anomaly detection
 *   description_out: Output buffer for anomaly description (256 bytes)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_analyze_user_activity(security_monitor_t *monitor,
                                            uint32_t user_id,
                                            bool *anomaly_detected_out,
                                            char *description_out);

/* ====================================================================
 * Alert Rules
 * ==================================================================== */

/**
 * Add alert rule
 *
 * Args:
 *   monitor: Security monitor
 *   rule: Alert rule to add
 *
 * Returns:
 *   Rule ID on success, -1 on failure
 */
int security_monitor_add_alert_rule(security_monitor_t *monitor,
                                     const alert_rule_t *rule);

/**
 * Remove alert rule
 *
 * Args:
 *   monitor: Security monitor
 *   rule_id: ID of rule to remove
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_remove_alert_rule(security_monitor_t *monitor, int rule_id);

/**
 * List alert rules
 *
 * Args:
 *   monitor: Security monitor
 *   rules_out: Output array of alert rules
 *   max_rules: Maximum number of rules to return
 *   count_out: Actual number of rules returned
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_list_alert_rules(security_monitor_t *monitor,
                                       alert_rule_t *rules_out,
                                       size_t max_rules,
                                       size_t *count_out);

/* ====================================================================
 * IP Blocking
 * ==================================================================== */

/**
 * Block IP address
 *
 * Temporarily or permanently blocks an IP address from connecting.
 *
 * Args:
 *   monitor: Security monitor
 *   ip_address: IP address to block
 *   duration_seconds: Block duration (0 = permanent)
 *   reason: Reason for blocking
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_block_ip(security_monitor_t *monitor,
                               const char *ip_address,
                               int duration_seconds,
                               const char *reason);

/**
 * Unblock IP address
 *
 * Args:
 *   monitor: Security monitor
 *   ip_address: IP address to unblock
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_unblock_ip(security_monitor_t *monitor,
                                 const char *ip_address);

/**
 * Check if IP is blocked
 *
 * Args:
 *   monitor: Security monitor
 *   ip_address: IP address to check
 *
 * Returns:
 *   true if blocked, false otherwise
 */
bool security_monitor_is_ip_blocked(security_monitor_t *monitor,
                                     const char *ip_address);

/* ====================================================================
 * Statistics and Reporting
 * ==================================================================== */

/**
 * Security statistics
 */
typedef struct {
    uint64_t total_events;              /* Total security events */
    uint64_t critical_events;           /* Critical events */
    uint64_t blocked_ips;               /* Currently blocked IPs */
    uint64_t brute_force_attempts;      /* Brute force attempts detected */
    uint64_t anomalies_detected;        /* Anomalies detected */
    time_t last_event_time;             /* Last security event time */
} security_stats_t;

/**
 * Get security statistics
 *
 * Args:
 *   monitor: Security monitor
 *   stats_out: Output statistics structure
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_get_stats(security_monitor_t *monitor,
                                security_stats_t *stats_out);

/**
 * Generate security report
 *
 * Generates a human-readable security report for the specified time period.
 *
 * Args:
 *   monitor: Security monitor
 *   start_time: Report start time
 *   end_time: Report end time
 *   output_file: Path to output file
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int security_monitor_generate_report(security_monitor_t *monitor,
                                      time_t start_time,
                                      time_t end_time,
                                      const char *output_file);

/**
 * Get default security monitor configuration
 *
 * Returns recommended settings for HIPAA compliance
 */
anomaly_config_t security_monitor_get_default_config(void);

#endif /* HL7DB_SECURITY_MONITOR_H */
