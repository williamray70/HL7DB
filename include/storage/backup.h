/**
 * backup.h - Backup and Disaster Recovery
 *
 * HIPAA Compliance: 45 CFR §164.308(a)(7) - Contingency Plan
 *
 * Implements:
 * - Data backup (§164.308(a)(7)(ii)(A))
 * - Disaster recovery plan (§164.308(a)(7)(ii)(B))
 * - Emergency mode operation (§164.308(a)(7)(ii)(C))
 * - Data backup encryption
 * - 6-year retention for audit logs
 */

#ifndef HL7DB_BACKUP_H
#define HL7DB_BACKUP_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ====================================================================
 * Backup Configuration
 * ==================================================================== */

/**
 * Backup types
 */
typedef enum {
    BACKUP_TYPE_FULL,       /* Full backup of all data */
    BACKUP_TYPE_INCREMENTAL /* Only changed data since last backup */
} backup_type_t;

/**
 * Backup configuration
 */
typedef struct {
    char backup_dir[256];           /* Directory for backups */
    char data_dir[256];             /* Source data directory */
    backup_type_t default_type;     /* Default backup type */
    bool encrypt_backups;           /* Encrypt backup files (HIPAA recommended) */
    bool compress_backups;          /* Compress backup files */
    int retention_days;             /* Days to retain backups (6 years for audit = 2190) */
    char encryption_key_file[256];  /* Encryption key for backups */
} backup_config_t;

/**
 * Backup metadata
 */
typedef struct {
    char backup_id[64];             /* Unique backup ID */
    backup_type_t type;             /* Backup type */
    time_t timestamp;               /* Backup timestamp */
    uint64_t size_bytes;            /* Backup size in bytes */
    uint64_t file_count;            /* Number of files backed up */
    bool encrypted;                 /* Is backup encrypted? */
    bool compressed;                /* Is backup compressed? */
    char checksum[65];              /* SHA-256 checksum of backup */
} backup_metadata_t;

/* ====================================================================
 * Backup System
 * ==================================================================== */

typedef struct backup_system backup_system_t;

/**
 * Initialize backup system
 *
 * Args:
 *   config: Backup configuration
 *
 * Returns:
 *   Backup system instance, or NULL on failure
 */
backup_system_t *backup_system_init(const backup_config_t *config);

/**
 * Destroy backup system
 *
 * Args:
 *   sys: Backup system instance
 */
void backup_system_destroy(backup_system_t *sys);

/* ====================================================================
 * Backup Operations
 * ==================================================================== */

/**
 * Create backup
 *
 * Args:
 *   sys: Backup system
 *   type: Backup type
 *   metadata_out: Output backup metadata
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int backup_create(backup_system_t *sys,
                  backup_type_t type,
                  backup_metadata_t *metadata_out);

/**
 * Restore from backup
 *
 * Args:
 *   sys: Backup system
 *   backup_id: ID of backup to restore
 *   verify_checksum: Verify backup integrity before restore
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int backup_restore(backup_system_t *sys,
                   const char *backup_id,
                   bool verify_checksum);

/**
 * List available backups
 *
 * Args:
 *   sys: Backup system
 *   backups_out: Output array of backup metadata
 *   max_backups: Maximum number of backups to return
 *   count_out: Actual number of backups returned
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int backup_list(backup_system_t *sys,
                backup_metadata_t *backups_out,
                size_t max_backups,
                size_t *count_out);

/**
 * Verify backup integrity
 *
 * Checks that backup file is not corrupted and checksum matches.
 *
 * Args:
 *   sys: Backup system
 *   backup_id: ID of backup to verify
 *
 * Returns:
 *   true if backup is valid, false otherwise
 */
bool backup_verify(backup_system_t *sys, const char *backup_id);

/**
 * Delete old backups based on retention policy
 *
 * Args:
 *   sys: Backup system
 *   deleted_count_out: Number of backups deleted
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int backup_cleanup_old(backup_system_t *sys, size_t *deleted_count_out);

/* ====================================================================
 * Scheduled Backups
 * ==================================================================== */

/**
 * Backup schedule configuration
 */
typedef struct {
    bool enabled;                   /* Enable scheduled backups */
    int interval_hours;             /* Backup interval in hours */
    backup_type_t backup_type;      /* Type of backup to create */
    int full_backup_day;            /* Day of week for full backup (0=Sunday) */
} backup_schedule_t;

/**
 * Start scheduled backup thread
 *
 * Args:
 *   sys: Backup system
 *   schedule: Backup schedule configuration
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int backup_start_scheduler(backup_system_t *sys, const backup_schedule_t *schedule);

/**
 * Stop scheduled backup thread
 *
 * Args:
 *   sys: Backup system
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int backup_stop_scheduler(backup_system_t *sys);

/* ====================================================================
 * Disaster Recovery
 * ==================================================================== */

/**
 * Export system configuration for disaster recovery
 *
 * Exports all configuration, user accounts, roles, and audit logs
 * to a separate location for disaster recovery.
 *
 * Args:
 *   sys: Backup system
 *   export_path: Path for export directory
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int backup_export_disaster_recovery(backup_system_t *sys, const char *export_path);

/**
 * Validate disaster recovery plan
 *
 * Performs a test restore to verify the disaster recovery plan works.
 * Does not overwrite production data.
 *
 * Args:
 *   sys: Backup system
 *   test_dir: Temporary directory for test restore
 *
 * Returns:
 *   0 on success (DR plan valid), -1 on failure
 */
int backup_test_disaster_recovery(backup_system_t *sys, const char *test_dir);

/* ====================================================================
 * Audit Log Archival (HIPAA 6-Year Retention)
 * ==================================================================== */

/**
 * Archive audit logs to long-term storage
 *
 * HIPAA requires 6-year retention of audit logs.
 * This function archives old audit logs to separate storage.
 *
 * Args:
 *   sys: Backup system
 *   audit_log_path: Path to audit log file
 *   archive_dir: Directory for archived logs
 *   older_than_days: Archive logs older than this many days
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int backup_archive_audit_logs(backup_system_t *sys,
                               const char *audit_log_path,
                               const char *archive_dir,
                               int older_than_days);

/**
 * Get default backup configuration
 *
 * Returns HIPAA-compliant defaults:
 * - Encrypted backups
 * - 6-year retention for audit logs
 * - Daily incremental, weekly full backups
 */
backup_config_t backup_get_default_config(void);

#endif /* HL7DB_BACKUP_H */
