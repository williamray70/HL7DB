# HIPAA Authentication & Audit Logging

## Overview

HL7DB implements comprehensive **Authentication, Authorization, and Audit Logging** to comply with HIPAA Security Rule requirements:

- **45 CFR §164.312(a)** - Access Control
- **45 CFR §164.312(d)** - Person or Entity Authentication
- **45 CFR §164.312(b)** - Audit Controls

## Features Implemented

### ✅ Authentication System (`security/auth.h`)

1. **User Account Management**
   - Unique user identification
   - Secure password hashing (PBKDF2-HMAC-SHA256, 100,000 iterations)
   - Account lockout after 5 failed attempts (15-minute lockout)
   - Force password change on first login
   - Password change functionality

2. **Role-Based Access Control (RBAC)**
   - Predefined roles: readonly, clinician, analyst, integration, admin, emergency
   - Fine-grained permissions (14 permission types)
   - Permission inheritance from roles
   - Custom role support

3. **Session Management**
   - Cryptographically secure session tokens (32 bytes random)
   - Automatic session timeout (15 minutes per HIPAA recommendations)
   - Session activity tracking
   - Concurrent session support

4. **Emergency Access (Break-Glass)**
   - Special emergency access procedure
   - Requires explicit reason (logged)
   - Enhanced audit trail
   - Separate permission flag

### ✅ Audit Logging System (`security/audit.h`)

1. **Comprehensive Event Logging**
   - 27 event types covering all security-relevant activities
   - WHO: User ID, username, session token
   - WHAT: Resource, action, PHI indicator
   - WHEN: Timestamp
   - WHERE: Client IP, hostname
   - WHY: Details, reason
   - RESULT: Success/failure, error code

2. **Tamper-Evident Logging**
   - Sequential event IDs
   - SHA-256 checksums for each event
   - Event chaining (links to previous event)
   - Append-only log file
   - Integrity verification support

3. **PHI Access Tracking**
   - All PHI access events logged
   - Record count tracking
   - Separate PHI access flag
   - Emergency access specially flagged

4. **Retention & Compliance**
   - JSON-formatted log entries (machine-readable)
   - Export functionality for archival
   - 6-year retention support (HIPAA requirement)
   - Query/analysis capabilities

## API Reference

### Authentication

#### Initialize System

```c
#include "security/auth.h"

auth_system_t *auth = auth_system_init();
```

Creates default admin user (username: `admin`, password: `changeme123!`)

#### Create Users

```c
uint32_t user_id = auth_create_user(
    auth,
    "jsmith",                    // username
    "SecurePass123!",            // password
    "Dr. John Smith",            // full name
    ROLE_CLINICIAN,              // role
    admin_session_token          // created by (for audit)
);
```

#### Login

```c
char *session_token = auth_login(
    auth,
    "jsmith",                    // username
    "SecurePass123!",            // password
    "192.168.1.100",             // client IP
    "HL7Interface/1.0"           // client info
);

if (session_token) {
    // Login successful
    // Use session_token for subsequent requests
} else {
    // Login failed
}
```

#### Emergency Access

```c
char *emergency_token = auth_emergency_login(
    auth,
    "emergency_user",
    "password",
    "Patient critical condition, immediate access required",  // REQUIRED reason
    "192.168.1.200",
    "EmergencyTerminal"
);
```

#### Validate Session

```c
const auth_session_t *session = auth_validate_session(auth, session_token);
if (session) {
    // Session valid
    printf("User: %s\n", session->username);
    printf("Permissions: 0x%x\n", session->permissions);
}
```

#### Check Permissions

```c
if (auth_has_permission(auth, session_token, PERM_READ_PHI)) {
    // User authorized to read PHI
} else {
    // Access denied
}
```

#### Logout

```c
auth_logout(auth, session_token);
free(session_token);
```

### Roles & Permissions

#### Predefined Roles

| Role | Permissions | Use Case |
|------|------------|----------|
| `ROLE_READONLY` | SELECT, LIST | Read-only access, no PHI |
| `ROLE_CLINICIAN` | READ_PHI, WRITE_PHI, SELECT, INSERT | Healthcare providers |
| `ROLE_ANALYST` | READ_PHI, SELECT, AUDIT_VIEW | Data analysts, researchers |
| `ROLE_INTEGRATION` | READ_PHI, WRITE_PHI, CRUD operations | System integrations |
| `ROLE_ADMIN` | ALL | System administrators |
| `ROLE_EMERGENCY` | READ_PHI, WRITE_PHI, EMERGENCY_ACCESS | Break-glass access |

#### Permission Flags

```c
PERM_READ_PHI           // Read Protected Health Information
PERM_WRITE_PHI          // Insert/modify PHI
PERM_DELETE_PHI         // Delete PHI (highly restricted)
PERM_QUERY_SELECT       // Execute SELECT queries
PERM_QUERY_INSERT       // Execute INSERT queries
PERM_QUERY_DELETE       // Execute DELETE queries
PERM_CHANNEL_CREATE     // Create channels
PERM_CHANNEL_DROP       // Drop channels
PERM_CHANNEL_LIST       // List channels
PERM_USER_MANAGE        // Manage user accounts
PERM_ROLE_MANAGE        // Manage roles
PERM_AUDIT_VIEW         // View audit logs
PERM_SYSTEM_CONFIG      // Modify system configuration
PERM_EMERGENCY_ACCESS   // Emergency break-glass access
```

### Audit Logging

#### Initialize Audit System

```c
#include "security/audit.h"

audit_system_t *audit = audit_system_init(
    "/var/log/hl7db/audit.log",  // log file path (NULL = default)
    1000                          // buffer size
);
```

#### Log Authentication Events

```c
audit_log_auth(
    audit,
    AUDIT_LOGIN_SUCCESS,         // event type
    "jsmith",                    // username
    "192.168.1.100",             // client IP
    true,                        // success
    "Login successful"           // details
);
```

#### Log PHI Access

```c
audit_log_phi_access(
    audit,
    AUDIT_PHI_READ,              // event type
    user_id,
    username,
    session_token,
    "adt_messages",              // resource (channel name)
    5,                           // number of records accessed
    client_ip,
    "SELECT * FROM adt_messages WHERE PID-3 = [REDACTED]"  // PHI MUST be redacted!
);
```

#### Log Queries

```c
audit_log_query(
    audit,
    user_id,
    username,
    session_token,
    "SELECT * FROM adt_messages",  // query (with PHI redacted)
    "adt_messages",                // channel
    10,                            // rows affected
    true,                          // PHI accessed?
    true,                          // success?
    client_ip
);
```

#### Log Access Denied

```c
audit_log_access_denied(
    audit,
    user_id,
    username,
    session_token,
    "lab_results",               // resource
    "SELECT",                    // action
    "Missing PERM_READ_PHI",     // reason
    client_ip
);
```

#### Log Emergency Access

```c
audit_log_emergency_access(
    audit,
    user_id,
    username,
    emergency_session_token,
    "Patient cardiac arrest, immediate chart access needed",
    client_ip
);
```

## Integration Example

### Complete Workflow

```c
#include "security/auth.h"
#include "security/audit.h"
#include "query/hl7sql_parser.h"
#include "util/phi_redaction.h"

void process_query_request(const char *username,
                           const char *password,
                           const char *query,
                           const char *client_ip) {
    /* Initialize systems */
    auth_system_t *auth = auth_system_init();
    audit_system_t *audit = auth->audit;  // Shared audit system
    hl7sql_channel_manager_t *mgr = hl7sql_channel_manager_create();

    /* Step 1: Authenticate */
    char *session_token = auth_login(auth, username, password, client_ip, "App/1.0");
    if (!session_token) {
        audit_log_auth(audit, AUDIT_LOGIN_FAILED, username, client_ip,
                      false, "Invalid credentials");
        printf("Authentication failed\n");
        return;
    }

    /* Step 2: Validate session */
    const auth_session_t *session = auth_validate_session(auth, session_token);
    if (!session) {
        printf("Session invalid or expired\n");
        return;
    }

    /* Step 3: Check permissions (example: SELECT query requires PERM_QUERY_SELECT) */
    if (!auth_has_permission(auth, session_token, PERM_QUERY_SELECT)) {
        audit_log_access_denied(audit, session->user_id, username,
                               session_token, "query", "SELECT",
                               "Missing PERM_QUERY_SELECT", client_ip);
        printf("Permission denied\n");
        free(session_token);
        return;
    }

    /* Step 4: Execute query */
    hl7sql_result_t *result = hl7sql_query(mgr, query);

    /* Step 5: Audit the query */
    bool phi_accessed = (result && result->num_rows > 0);  // Simplified check
    audit_log_query(audit, session->user_id, username, session_token,
                   query,  // Should redact PHI in query!
                   "adt_messages",  // Channel from query parsing
                   result ? result->num_rows : 0,
                   phi_accessed,
                   result && result->error_code == 0,
                   client_ip);

    /* Step 6: Display results (with PHI redaction) */
    if (result && result->error_code == 0) {
        phi_redaction_config_t config = phi_redaction_get_default_config();
        hl7sql_result_print_redacted(result, &config);

        /* Audit PHI access if query returned data */
        if (phi_accessed) {
            audit_log_phi_access(audit, AUDIT_PHI_READ,
                                session->user_id, username, session_token,
                                "adt_messages", result->num_rows,
                                client_ip, "Query executed");
        }
    }

    /* Step 7: Cleanup */
    hl7sql_result_destroy(result);
    auth_logout(auth, session_token);
    free(session_token);

    hl7sql_channel_manager_destroy(mgr);
    auth_system_destroy(auth);
}
```

## Audit Log Format

Audit events are logged in JSON format:

```json
{"event_id":1,"timestamp":1781410191,"type":"LOGIN_SUCCESS","severity":"INFO","user_id":2,"username":"jsmith","session":"a3f2b1c4...","client_ip":"192.168.1.100","resource":"","action":"authenticate","details":"Login successful from 192.168.1.100","success":true,"phi_accessed":false,"phi_record_count":0,"checksum":"a3f2b1c4d5e6f7g8..."}
{"event_id":2,"timestamp":1781410195,"type":"PHI_READ","severity":"INFO","user_id":2,"username":"jsmith","session":"a3f2b1c4...","client_ip":"192.168.1.100","resource":"adt_messages","action":"phi_access","details":"SELECT query executed","success":true,"phi_accessed":true,"phi_record_count":5,"checksum":"b4c5d6e7f8g9h0i1..."}
```

### Event Fields

- `event_id`: Sequential event ID (1, 2, 3, ...)
- `timestamp`: Unix timestamp
- `type`: Event type (LOGIN_SUCCESS, PHI_READ, etc.)
- `severity`: INFO, WARNING, ERROR, CRITICAL
- `user_id`: User ID (0 for system events)
- `username`: Username
- `session`: Session token (first 32 chars)
- `client_ip`: Client IP address
- `resource`: Resource accessed (channel, user, etc.)
- `action`: Action performed
- `details`: Additional details
- `success`: Operation succeeded?
- `phi_accessed`: PHI was accessed?
- `phi_record_count`: Number of PHI records
- `checksum`: SHA-256 checksum (tamper detection)

## Security Best Practices

### Password Policy

```c
// Minimum 8 characters (enforced)
// Recommend: uppercase, lowercase, numbers, symbols
// Force password change for new users
// Regular password expiration (implement externally)
```

### Session Management

```c
// Automatic 15-minute timeout (HIPAA recommended)
// Session tokens: 32 bytes cryptographically random
// Destroy sessions on logout
// Cleanup expired sessions periodically

auth_cleanup_expired_sessions(auth);
```

### Audit Requirements

```c
// Log ALL PHI access (read/write/delete)
// Log authentication attempts (success and failure)
// Log permission denials
// Log emergency access with reason
// Never log PHI in audit logs (redact first!)

// Example: BAD
audit_log_query(audit, ..., "SELECT * FROM WHERE PID-3 = '123456789'");

// Example: GOOD
phi_redaction_config_t config = phi_redaction_get_default_config();
char *safe_query = phi_redact_message(query, &config);
audit_log_query(audit, ..., safe_query);
free(safe_query);
```

### Account Lockout

- 5 failed attempts → 15-minute lockout (automatic)
- Admin can manually lock/unlock accounts
- Lockouts are logged

### Emergency Access

```c
// Only for true emergencies
// Requires PERM_EMERGENCY_ACCESS
// Mandatory reason field
// CRITICAL severity in audit log
// Post-emergency review required

char *token = auth_emergency_login(
    auth, "emergency_user", "pass",
    "Patient code blue, immediate chart access", // Required!
    client_ip, client_info
);
```

## Compliance Checklist

- [x] Unique user identification (45 CFR §164.312(a)(2)(i))
- [x] Emergency access procedure (45 CFR §164.312(a)(2)(ii))
- [x] Automatic logoff after 15 minutes (45 CFR §164.312(a)(2)(iii))
- [x] Person/entity authentication (45 CFR §164.312(d))
- [x] Audit controls - record PHI access (45 CFR §164.312(b))
- [x] Secure password storage (PBKDF2, not plaintext)
- [x] Account lockout after failed attempts
- [x] Role-based access control
- [x] Session management with timeout
- [x] Comprehensive audit trail
- [x] Tamper-evident audit logs

## Known Limitations & Future Work

1. **Password Complexity**: Currently enforces minimum length only. Add complexity requirements.

2. **MFA (Multi-Factor Authentication)**: Not implemented. Should add TOTP/U2F support.

3. **Audit Log Analysis**: Basic logging implemented. Add:
   - Full query/filter capabilities
   - Statistical analysis
   - Anomaly detection
   - Automated alerts

4. **Certificate-Based Auth**: Not implemented. Should support X.509 certificates.

5. **Session Persistence**: Sessions lost on restart. Consider persistent storage.

6. **Audit Log Encryption**: Logs are not encrypted at rest. Should encrypt.

7. **Compliance Reporting**: Add automated HIPAA compliance reports.

## Testing

A comprehensive test suite should validate:

```bash
# Test authentication
- User creation
- Login success/failure
- Account lockout
- Password change
- Session timeout
- Emergency access

# Test authorization
- Permission checks for each role
- Permission denial logging
- Role-based access

# Test audit logging
- All event types logged
- PHI access tracking
- Checksum integrity
- Log format correctness

# Integration tests
- Auth + Audit + Query execution
- Complete workflow testing
```

## References

- [HIPAA Security Rule](https://www.hhs.gov/hipaa/for-professionals/security/index.html)
- [45 CFR §164.312](https://www.ecfr.gov/current/title-45/subtitle-A/subchapter-C/part-164/subpart-C/section-164.312)
- [NIST SP 800-66: HIPAA Security Rule Guide](https://csrc.nist.gov/publications/detail/sp/800-66/rev-2/draft)
- [OWASP Authentication Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Authentication_Cheat_Sheet.html)
