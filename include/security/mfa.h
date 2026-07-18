/**
 * mfa.h - Multi-Factor Authentication (TOTP)
 *
 * HIPAA Compliance Enhancement:
 * - Two-factor authentication for enhanced security
 * - Time-based One-Time Password (TOTP) per RFC 6238
 * - QR code generation for easy mobile app setup
 */

#ifndef HL7DB_MFA_H
#define HL7DB_MFA_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ====================================================================
 * TOTP Configuration
 * ==================================================================== */

#define MFA_SECRET_SIZE 20          /* 160-bit secret (Base32 encoded) */
#define MFA_SECRET_B32_SIZE 33      /* Base32-encoded secret length + null */
#define MFA_CODE_LENGTH 6           /* 6-digit TOTP codes */
#define MFA_TIME_STEP 30            /* 30-second time step */
#define MFA_WINDOW 1                /* Allow ±1 time step for clock skew */

/**
 * MFA user data
 */
typedef struct {
    uint32_t user_id;                       /* User ID */
    char secret[MFA_SECRET_B32_SIZE];       /* Base32-encoded TOTP secret */
    bool enabled;                           /* MFA enabled for this user? */
    bool verified;                          /* Has user verified MFA setup? */
    time_t enrolled_at;                     /* When MFA was enrolled */
    uint32_t backup_codes_used;             /* Bitmask of used backup codes */
} mfa_user_data_t;

/* ====================================================================
 * MFA System
 * ==================================================================== */

typedef struct mfa_system mfa_system_t;

/**
 * Initialize MFA system
 *
 * Returns:
 *   MFA system instance, or NULL on failure
 */
mfa_system_t *mfa_system_init(void);

/**
 * Destroy MFA system
 *
 * Args:
 *   sys: MFA system instance
 */
void mfa_system_destroy(mfa_system_t *sys);

/* ====================================================================
 * MFA Enrollment
 * ==================================================================== */

/**
 * Enroll user in MFA
 *
 * Generates a new TOTP secret for the user.
 *
 * Args:
 *   sys: MFA system
 *   user_id: User ID
 *   secret_out: Output buffer for Base32-encoded secret (MFA_SECRET_B32_SIZE bytes)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int mfa_enroll_user(mfa_system_t *sys, uint32_t user_id, char *secret_out);

/**
 * Get provisioning URI for QR code
 *
 * Generates a URI suitable for encoding in a QR code for mobile apps
 * like Google Authenticator, Authy, etc.
 *
 * Format: otpauth://totp/HL7DB:username?secret=SECRET&issuer=HL7DB
 *
 * Args:
 *   username: Username for display
 *   secret: Base32-encoded TOTP secret
 *   uri_out: Output buffer for URI (256 bytes recommended)
 *   uri_len: Size of output buffer
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int mfa_get_provisioning_uri(const char *username,
                              const char *secret,
                              char *uri_out,
                              size_t uri_len);

/**
 * Verify TOTP code during enrollment
 *
 * User must verify they can generate valid codes before MFA is enabled.
 *
 * Args:
 *   sys: MFA system
 *   user_id: User ID
 *   code: 6-digit TOTP code from user
 *
 * Returns:
 *   0 on success (enables MFA), -1 on failure
 */
int mfa_verify_enrollment(mfa_system_t *sys, uint32_t user_id, const char *code);

/**
 * Disable MFA for user
 *
 * Args:
 *   sys: MFA system
 *   user_id: User ID
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int mfa_disable_user(mfa_system_t *sys, uint32_t user_id);

/* ====================================================================
 * MFA Verification
 * ==================================================================== */

/**
 * Verify TOTP code for user
 *
 * Checks if the provided code is valid for the current time window.
 * Allows for clock skew (±MFA_WINDOW time steps).
 *
 * Args:
 *   sys: MFA system
 *   user_id: User ID
 *   code: 6-digit TOTP code from user
 *
 * Returns:
 *   true if valid, false otherwise
 */
bool mfa_verify_code(mfa_system_t *sys, uint32_t user_id, const char *code);

/**
 * Check if MFA is enabled for user
 *
 * Args:
 *   sys: MFA system
 *   user_id: User ID
 *
 * Returns:
 *   true if MFA enabled and verified, false otherwise
 */
bool mfa_is_enabled(mfa_system_t *sys, uint32_t user_id);

/* ====================================================================
 * Backup Codes
 * ==================================================================== */

#define MFA_BACKUP_CODE_COUNT 10
#define MFA_BACKUP_CODE_LENGTH 10

/**
 * Generate backup codes for user
 *
 * Backup codes allow account recovery if TOTP device is lost.
 * These should be stored securely by the user.
 *
 * Args:
 *   sys: MFA system
 *   user_id: User ID
 *   codes_out: Array of backup codes (10 codes, each MFA_BACKUP_CODE_LENGTH+1 bytes)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int mfa_generate_backup_codes(mfa_system_t *sys,
                               uint32_t user_id,
                               char codes_out[MFA_BACKUP_CODE_COUNT][MFA_BACKUP_CODE_LENGTH + 1]);

/**
 * Verify backup code
 *
 * Backup codes are single-use only.
 *
 * Args:
 *   sys: MFA system
 *   user_id: User ID
 *   code: Backup code from user
 *
 * Returns:
 *   true if valid (and marks as used), false otherwise
 */
bool mfa_verify_backup_code(mfa_system_t *sys, uint32_t user_id, const char *code);

/* ====================================================================
 * TOTP Core Functions (Low-Level)
 * ==================================================================== */

/**
 * Generate TOTP code for current time
 *
 * Args:
 *   secret: Base32-encoded secret
 *   time_step: Current time step (use time(NULL) / MFA_TIME_STEP)
 *   code_out: Output buffer for 6-digit code (7 bytes minimum)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int totp_generate_code(const char *secret, uint64_t time_step, char *code_out);

/**
 * Verify TOTP code against secret
 *
 * Args:
 *   secret: Base32-encoded secret
 *   code: 6-digit code to verify
 *   window: Time window for clock skew (±window time steps)
 *
 * Returns:
 *   true if valid, false otherwise
 */
bool totp_verify_code(const char *secret, const char *code, int window);

#endif /* HL7DB_MFA_H */
