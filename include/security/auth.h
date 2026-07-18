/**
 * auth.h - Authentication & Authorization System
 *
 * HIPAA Security Rule Compliance:
 * - 45 CFR §164.312(a)(1) - Access Control
 * - 45 CFR §164.312(a)(2)(i) - Unique User Identification
 * - 45 CFR §164.312(a)(2)(ii) - Emergency Access Procedure
 * - 45 CFR §164.312(a)(2)(iii) - Automatic Logoff
 * - 45 CFR §164.312(d) - Person or Entity Authentication
 *
 * Implements:
 * - User accounts with secure password hashing (bcrypt/PBKDF2)
 * - Role-Based Access Control (RBAC)
 * - Session management with automatic timeout
 * - Emergency access procedures
 * - Comprehensive audit trail
 */

#ifndef HL7DB_AUTH_H
#define HL7DB_AUTH_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ====================================================================
 * Permission Flags (Bitfield)
 * ==================================================================== */

typedef enum {
    PERM_NONE               = 0,

    /* Data Access Permissions */
    PERM_READ_PHI           = 1 << 0,   /* Read Protected Health Information */
    PERM_WRITE_PHI          = 1 << 1,   /* Insert/modify PHI data */
    PERM_DELETE_PHI         = 1 << 2,   /* Delete PHI (rare, highly restricted) */

    /* Query Permissions */
    PERM_QUERY_SELECT       = 1 << 3,   /* Execute SELECT queries */
    PERM_QUERY_INSERT       = 1 << 4,   /* Execute INSERT queries */
    PERM_QUERY_DELETE       = 1 << 5,   /* Execute DELETE queries */

    /* Channel Management */
    PERM_CHANNEL_CREATE     = 1 << 6,   /* Create channels */
    PERM_CHANNEL_DROP       = 1 << 7,   /* Drop channels */
    PERM_CHANNEL_LIST       = 1 << 8,   /* List all channels */

    /* Administrative Permissions */
    PERM_USER_MANAGE        = 1 << 9,   /* Create/modify user accounts */
    PERM_ROLE_MANAGE        = 1 << 10,  /* Create/modify roles */
    PERM_AUDIT_VIEW         = 1 << 11,  /* View audit logs */
    PERM_SYSTEM_CONFIG      = 1 << 12,  /* Modify system configuration */

    /* Emergency Access */
    PERM_EMERGENCY_ACCESS   = 1 << 13,  /* Break-glass emergency access */

    /* All permissions (superuser) */
    PERM_ALL                = 0xFFFFFFFF
} permission_t;

/* ====================================================================
 * Predefined Roles
 * ==================================================================== */

typedef enum {
    ROLE_NONE = 0,
    ROLE_READONLY,          /* Read-only access, can view redacted data */
    ROLE_CLINICIAN,         /* Read PHI, insert messages */
    ROLE_ANALYST,           /* Read PHI, run queries, no write */
    ROLE_INTEGRATION,       /* System integration, read/write, no admin */
    ROLE_ADMIN,             /* Full administrative access */
    ROLE_EMERGENCY,         /* Emergency break-glass access */
    ROLE_CUSTOM             /* Custom role (defined by permissions) */
} role_type_t;

/* ====================================================================
 * User Account
 * ==================================================================== */

#define MAX_USERNAME_LEN 64
#define MAX_FULLNAME_LEN 128
#define BCRYPT_HASH_LEN 60      /* bcrypt hash length */
#define MAX_ROLES_PER_USER 8

typedef struct {
    uint32_t user_id;                   /* Unique user ID */
    char username[MAX_USERNAME_LEN];    /* Login username (unique) */
    char full_name[MAX_FULLNAME_LEN];   /* Full name for audit logs */
    char password_hash[BCRYPT_HASH_LEN + 1]; /* bcrypt hash */

    role_type_t primary_role;           /* Primary role */
    uint32_t permissions;               /* Permission bitfield */

    bool is_active;                     /* Account enabled? */
    bool is_locked;                     /* Account locked? */
    bool force_password_change;         /* Force password change on next login */

    time_t created_at;                  /* Account creation timestamp */
    time_t last_login;                  /* Last successful login */
    time_t password_changed_at;         /* Last password change */

    uint32_t failed_login_attempts;     /* Failed login counter */
    time_t locked_until;                /* Lockout expiration time */
} auth_user_t;

/* ====================================================================
 * Session Management
 * ==================================================================== */

#define SESSION_TOKEN_LEN 32
#define SESSION_TIMEOUT_SECONDS 900     /* 15 minutes (HIPAA recommended) */
#define MAX_CONCURRENT_SESSIONS 10

typedef struct {
    char token[SESSION_TOKEN_LEN * 2 + 1]; /* Hex-encoded token */
    uint32_t user_id;                   /* User ID */
    char username[MAX_USERNAME_LEN];    /* Username (for logging) */
    uint32_t permissions;               /* Cached permissions */

    time_t created_at;                  /* Session creation time */
    time_t last_activity;               /* Last activity timestamp */
    time_t expires_at;                  /* Session expiration time */

    bool is_emergency;                  /* Emergency access session? */
    char client_ip[64];                 /* Client IP address */
    char client_info[128];              /* Client information */
} auth_session_t;

/* ====================================================================
 * Authentication System
 * ==================================================================== */

typedef struct auth_system auth_system_t;

/**
 * Initialize authentication system
 *
 * Creates the default admin user if no users exist.
 *
 * Returns:
 *   Pointer to auth system, or NULL on failure
 */
auth_system_t *auth_system_init(void);

/**
 * Get audit system from auth system
 *
 * Args:
 *   sys: Authentication system
 *
 * Returns:
 *   Audit system pointer
 */
struct audit_system *auth_get_audit_system(auth_system_t *sys);

/**
 * Shutdown authentication system
 *
 * Closes all sessions and frees resources.
 *
 * Args:
 *   sys: Authentication system
 */
void auth_system_destroy(auth_system_t *sys);

/* ====================================================================
 * User Management
 * ==================================================================== */

/**
 * Create a new user account
 *
 * Password is hashed using bcrypt before storage.
 * Requires PERM_USER_MANAGE permission.
 *
 * Args:
 *   sys: Authentication system
 *   username: Login username (must be unique)
 *   password: Plain text password (will be hashed)
 *   full_name: User's full name
 *   role: Primary role
 *   created_by_session: Session token of creator (for audit)
 *
 * Returns:
 *   User ID on success, 0 on failure
 */
uint32_t auth_create_user(auth_system_t *sys,
                           const char *username,
                           const char *password,
                           const char *full_name,
                           role_type_t role,
                           const char *created_by_session);

/**
 * Authenticate user and create session
 *
 * Checks username/password and creates a session token.
 * Implements account lockout after failed attempts.
 *
 * Args:
 *   sys: Authentication system
 *   username: Username
 *   password: Plain text password
 *   client_ip: Client IP address (for audit)
 *   client_info: Client information string
 *
 * Returns:
 *   Session token (caller must free), or NULL on failure
 */
char *auth_login(auth_system_t *sys,
                 const char *username,
                 const char *password,
                 const char *client_ip,
                 const char *client_info);

/**
 * Emergency access login (break-glass)
 *
 * Provides emergency access with enhanced audit logging.
 * Requires PERM_EMERGENCY_ACCESS on the user account.
 *
 * Args:
 *   sys: Authentication system
 *   username: Username
 *   password: Password
 *   reason: Emergency access reason (required, logged)
 *   client_ip: Client IP address
 *   client_info: Client information
 *
 * Returns:
 *   Session token (caller must free), or NULL on failure
 */
char *auth_emergency_login(auth_system_t *sys,
                           const char *username,
                           const char *password,
                           const char *reason,
                           const char *client_ip,
                           const char *client_info);

/**
 * Logout and invalidate session
 *
 * Args:
 *   sys: Authentication system
 *   session_token: Session token to invalidate
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int auth_logout(auth_system_t *sys, const char *session_token);

/**
 * Change user password
 *
 * Args:
 *   sys: Authentication system
 *   session_token: Current session token
 *   old_password: Current password
 *   new_password: New password
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int auth_change_password(auth_system_t *sys,
                         const char *session_token,
                         const char *old_password,
                         const char *new_password);

/**
 * Lock/unlock user account
 *
 * Requires PERM_USER_MANAGE permission.
 *
 * Args:
 *   sys: Authentication system
 *   admin_session: Admin session token
 *   username: Username to lock/unlock
 *   lock: true to lock, false to unlock
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int auth_set_user_locked(auth_system_t *sys,
                         const char *admin_session,
                         const char *username,
                         bool lock);

/* ====================================================================
 * Session Management
 * ==================================================================== */

/**
 * Validate session token and check for timeout
 *
 * Updates last_activity timestamp on valid session.
 *
 * Args:
 *   sys: Authentication system
 *   session_token: Session token to validate
 *
 * Returns:
 *   Session pointer (internal, do not free), or NULL if invalid/expired
 */
const auth_session_t *auth_validate_session(auth_system_t *sys,
                                              const char *session_token);

/**
 * Check if session has specific permission
 *
 * Args:
 *   sys: Authentication system
 *   session_token: Session token
 *   permission: Permission to check
 *
 * Returns:
 *   true if authorized, false otherwise
 */
bool auth_has_permission(auth_system_t *sys,
                         const char *session_token,
                         permission_t permission);

/**
 * Cleanup expired sessions
 *
 * Should be called periodically (e.g., every minute).
 *
 * Args:
 *   sys: Authentication system
 *
 * Returns:
 *   Number of sessions expired
 */
int auth_cleanup_expired_sessions(auth_system_t *sys);

/* ====================================================================
 * Role & Permission Management
 * ==================================================================== */

/**
 * Get permissions for a predefined role
 *
 * Args:
 *   role: Role type
 *
 * Returns:
 *   Permission bitfield
 */
uint32_t auth_get_role_permissions(role_type_t role);

/**
 * Get role name as string
 *
 * Args:
 *   role: Role type
 *
 * Returns:
 *   Role name string
 */
const char *auth_role_to_string(role_type_t role);

/**
 * Get permission name as string
 *
 * Args:
 *   perm: Permission
 *
 * Returns:
 *   Permission name string
 */
const char *auth_permission_to_string(permission_t perm);

/* ====================================================================
 * Utility Functions
 * ==================================================================== */

/**
 * Hash password using bcrypt
 *
 * Args:
 *   password: Plain text password
 *   hash_out: Output buffer (at least BCRYPT_HASH_LEN + 1 bytes)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int auth_hash_password(const char *password, char *hash_out);

/**
 * Verify password against bcrypt hash
 *
 * Args:
 *   password: Plain text password
 *   hash: bcrypt hash
 *
 * Returns:
 *   true if password matches, false otherwise
 */
bool auth_verify_password(const char *password, const char *hash);

/**
 * Generate cryptographically secure random session token
 *
 * Args:
 *   token_out: Output buffer (at least SESSION_TOKEN_LEN * 2 + 1 bytes)
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int auth_generate_token(char *token_out);

#endif /* HL7DB_AUTH_H */
