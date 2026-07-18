# HL7DB HIPAA Compliance Guide

## Overview

HL7DB is an **HL7 message database and query system** configured with HIPAA Security Rule compliance features. This system stores and allows querying of HL7 messages - it does not modify medical records.

**Important:** HL7DB is a **read-only query system** for HL7 messages, not an Electronic Medical Record (EMR) system. The HIPAA requirements below reflect the actual obligations for this type of system.

## What HL7DB Does

- **Stores** HL7 messages in an encrypted database
- **Allows queries** to retrieve and search messages
- **Does NOT modify** medical records
- **Does NOT create** medical documentation

## HIPAA Security Rule Compliance

### ✓ §164.312(a)(2)(iv) - Encryption and Decryption

**Implementation:**
- **AES-256-GCM encryption** for all data at rest
- PBKDF2 key derivation with 100,000 iterations
- Unique initialization vectors (IV) per message
- Authentication tags for tamper detection
- Encrypted message format: `ENC|msg_id|timestamp|base64_encrypted_data`

**Configuration:**
- Encryption is **enabled by default**
- Master key location: `/var/hl7db/keys/master.key`
- File permissions: 0400 (read-only by owner)

**Why Required:** HL7 messages contain PHI (patient names, MRNs, diagnoses, etc.) that must be protected at rest.

### ✓ §164.312(e)(1) - Transmission Security

**Implementation:**
- **TLS 1.2+** for all network communications
- Strong cipher suites only:
  - ECDHE-RSA-AES256-GCM-SHA384
  - ECDHE-RSA-AES128-GCM-SHA256
  - DHE-RSA-AES256-GCM-SHA384
  - DHE-RSA-AES128-GCM-SHA256
- Optional client certificate verification

**Configuration:**
- TLS server runs on port 7778 (configurable)
- Certificates: `/etc/hl7db/server-cert.pem` and `/etc/hl7db/server-key.pem`

**Why Required:** PHI transmitted over the network must be encrypted to prevent interception.

### ✓ §164.312(d) - Person or Entity Authentication

**Implementation:**
- PBKDF2-HMAC-SHA256 password hashing (100,000 iterations)
- Role-Based Access Control (RBAC) with 7 predefined roles
- Account lockout after 5 failed attempts
- Session management with configurable timeout
- Password complexity enforcement
- **Optional TOTP-based Multi-Factor Authentication (RFC 6238)**

**Default Credentials:**
- Username: `admin`
- Password: `HL7db@Admin123!`
- **⚠️ CHANGE IMMEDIATELY AFTER FIRST LOGIN**

**Roles:**
1. ROLE_ADMIN - Full administrative access
2. ROLE_CLINICAL - Clinical data access
3. ROLE_BILLING - Billing data access
4. ROLE_AUDIT - Audit log access (read-only)
5. ROLE_READONLY - Read-only access
6. ROLE_EMERGENCY - Emergency break-glass access
7. ROLE_CUSTOM - Custom permissions

**Why Required:** Only authorized users should access PHI. Strong authentication prevents unauthorized access.

### ✓ §164.308(a)(7) - Contingency Plan (Data Backup)

**Implementation:**
- Automated backup system
- Full and incremental backup support
- **Daily incremental backups**
- **Weekly full backups** (Sunday)
- **90-day retention** (configurable)
- Backup encryption and compression
- Disaster recovery testing capability

**Configuration:**
- Backup directory: `/var/hl7db/backups`
- Backup manifest: `/var/hl7db/backups/backup_manifest.txt`

**Why Required:** HIPAA requires backup procedures to prevent data loss. 90-day retention is standard for disaster recovery backups.

**Note:** HL7 messages themselves don't have a HIPAA-mandated retention period. Retention of message data is based on your organization's business needs.

### ✓ §164.308(a)(1)(ii)(D) - Information System Activity Review

**Implementation:**
- Real-time security event monitoring
- Brute force attack detection
- Anomaly detection:
  - Unusual access hours (outside 8am-6pm)
  - Unusual query volume
  - Unusual access patterns
- IP blocking and rate limiting
- Alert system with multiple actions:
  - Log to file
  - Email notifications
  - Syslog integration
  - Webhook callbacks
  - Automatic account locking
  - Automatic IP blocking

**Default Rules:**
- Block IP after brute force attack (≥5 failed logins)
- Alert on unusual hours access
- Alert on 3x normal query volume

**Why Required:** HIPAA requires monitoring of system activity to detect security incidents.

## What HL7DB Does NOT Require

### ❌ Audit Logs of Medical Record Changes
**Not required because:** HL7DB is a query system that doesn't modify medical records. HIPAA audit requirements (§164.312(b)) apply to systems that create, modify, or delete medical documentation.

The audit logging system is **available** if you want to track user queries, but it's **optional** for HIPAA compliance since no medical records are being changed.

### ❌ 6-Year Retention of HL7 Messages
**Not required because:** HIPAA doesn't mandate retention periods for transient HL7 messages. Medical record retention requirements apply to the systems that create and maintain the official medical record (e.g., your EMR system).

Message retention should be based on:
- Your organization's policies
- State laws
- Business needs
- Typical retention: 30-90 days, or as needed

### ❌ 6-Year Retention of Backups
**Not required because:** Backups are for disaster recovery, not long-term archival. 90-day retention is standard for DR backups.

## Setup Instructions

### 1. Generate TLS Certificates

For testing/development:
```bash
./scripts/generate-certs.sh
```

For production, obtain certificates from a trusted Certificate Authority (CA).

### 2. Create Required Directories

```bash
sudo mkdir -p /var/hl7db/{keys,data,backups}
sudo mkdir -p /var/log/hl7db
sudo mkdir -p /etc/hl7db

# Set ownership (replace 'hl7db' with your service user)
sudo chown -R hl7db:hl7db /var/hl7db
sudo chown -R hl7db:hl7db /var/log/hl7db
sudo chmod 700 /var/hl7db/keys
sudo chmod 755 /var/hl7db/data
sudo chmod 755 /var/hl7db/backups
```

### 3. Generate Encryption Master Key

```bash
# The encryption system will auto-generate a key on first run
# Or manually create one:
openssl rand -hex 32 | sudo tee /var/hl7db/keys/master.key
sudo chmod 400 /var/hl7db/keys/master.key
sudo chown hl7db:hl7db /var/hl7db/keys/master.key
```

### 4. Start the Server

```bash
./bin/hl7db --port 7778
```

The server will display comprehensive security status on startup:

```
========================================
=== HL7DB Server Ready (HIPAA Mode) ===
========================================

Security Status:
  ✓ TLS 1.2+ server listening on port 7778
  ✓ AES-256-GCM encryption at rest
  ✓ PBKDF2-HMAC-SHA256 password hashing
  ✓ TOTP multi-factor authentication available
  ✓ Security monitoring and anomaly detection active
  ✓ Automated backups (daily incremental, weekly full)

HIPAA Compliance:
  ✓ §164.312(a)(2)(iv) - Encryption at Rest
  ✓ §164.312(e)(1) - Transmission Security (TLS)
  ✓ §164.312(d) - Authentication & Access Control
  ✓ §164.308(a)(7) - Backup & Disaster Recovery
  ✓ §164.308(a)(1)(ii)(D) - Security Monitoring
```

## Security Features

### Encryption at Rest

All messages are encrypted before being written to disk:

```
Format: ENC|<msg_id>|<timestamp>|<base64_encrypted_data>
Example: ENC|1|1234567890|aGVsbG8gd29ybGQ=...
```

- **Algorithm:** AES-256-GCM
- **Key Derivation:** PBKDF2-HMAC-SHA256 (100,000 iterations)
- **IV:** Unique per message (96 bits)
- **Authentication:** GCM authentication tag (128 bits)

### TLS Encryption

All network traffic is encrypted using TLS 1.2+:

- **Minimum Version:** TLS 1.2
- **Cipher Suites:** Strong forward-secrecy ciphers only
- **Certificate Verification:** Optional client certificates
- **Perfect Forward Secrecy:** ECDHE/DHE key exchange

### Password Security

- **Hashing:** PBKDF2-HMAC-SHA256
- **Iterations:** 100,000
- **Salt:** 256-bit unique per user
- **Minimum Length:** 8 characters
- **Complexity:** Mixed case, numbers, special characters required
- **Lockout:** 5 failed attempts

### Multi-Factor Authentication (Optional)

- **Algorithm:** TOTP (Time-based One-Time Password)
- **Standard:** RFC 6238
- **Code Length:** 6 digits
- **Time Step:** 30 seconds
- **Window:** ±1 time step for clock skew
- **Backup Codes:** 10 single-use recovery codes per user

## Backup and Recovery

### Creating Manual Backup

```bash
# Full backup
hl7db-backup create --type full

# Incremental backup
hl7db-backup create --type incremental
```

### Listing Backups

```bash
hl7db-backup list
```

### Restoring from Backup

```bash
hl7db-backup restore --id <backup_id> --verify-checksum
```

### Testing Disaster Recovery

```bash
hl7db-backup test-dr --test-dir /tmp/dr-test
```

### Configuring Retention

Edit the backup configuration to adjust retention period:

```c
backup_config_t config = {
    .retention_days = 90,  // Adjust as needed (default: 90 days)
    // ... other settings
};
```

## Security Monitoring

### View Security Statistics

```bash
# Check security events
hl7db-security stats

# Generate security report
hl7db-security report --start "2024-01-01" --end "2024-12-31" --output security-report.txt
```

### Configure Alert Rules

Alert rules are configured in the security monitor initialization. Example:

```c
alert_rule_t rule = {
    .event_type = SECURITY_EVENT_BRUTE_FORCE,
    .min_severity = SECURITY_SEVERITY_HIGH,
    .action = ALERT_ACTION_BLOCK_IP,
    .enabled = true
};
security_monitor_add_alert_rule(monitor, &rule);
```

### IP Blocking

```bash
# Block IP
hl7db-security block-ip 192.168.1.100 --duration 3600 --reason "Brute force attempt"

# Unblock IP
hl7db-security unblock-ip 192.168.1.100

# List blocked IPs
hl7db-security list-blocked
```

## Optional: Audit Logging

While not required for HIPAA compliance (since HL7DB doesn't modify medical records), audit logging is available if your organization wants to track queries and system access.

### Audit Log Format

```
[timestamp] [severity] [event_type] user=<username> ip=<ip_address> details=<details>
```

### Viewing Audit Logs

```bash
tail -f /var/log/hl7db/audit.log
```

### When to Enable Audit Logging

Consider enabling audit logging if:
- Your organization's policies require query tracking
- You need forensic investigation capabilities
- Regulatory requirements beyond HIPAA apply
- You want detailed access logs for compliance reporting

## User Management

### Creating Users

```c
uint32_t user_id = auth_create_user(
    auth_system,
    "jdoe",                    // username
    "SecureP@ssw0rd123",       // password
    "John Doe",                // full name
    ROLE_CLINICAL,             // role
    "admin"                    // created by session
);
```

### Enrolling Users in MFA

```c
char secret[MFA_SECRET_B32_SIZE];
mfa_enroll_user(mfa_system, user_id, secret);

// Generate QR code URI
char uri[256];
mfa_get_provisioning_uri("jdoe", secret, uri, sizeof(uri));

// User scans QR code with authenticator app
// User verifies with first code
mfa_verify_enrollment(mfa_system, user_id, "123456");
```

### Generating Backup Codes

```c
char backup_codes[MFA_BACKUP_CODE_COUNT][MFA_BACKUP_CODE_LENGTH + 1];
mfa_generate_backup_codes(mfa_system, user_id, backup_codes);

// Display to user (store securely)
for (int i = 0; i < MFA_BACKUP_CODE_COUNT; i++) {
    printf("Backup Code %d: %s\n", i+1, backup_codes[i]);
}
```

## Production Deployment Checklist

- [ ] Generate or obtain valid TLS certificates from trusted CA
- [ ] Change default admin password
- [ ] Configure automated backups
- [ ] Set up backup encryption key rotation
- [ ] Configure security monitoring alerts
- [ ] Set up email notifications for security events
- [ ] Set up firewall rules (allow only port 7778 for TLS)
- [ ] Enable MFA for all administrative users
- [ ] Document disaster recovery procedures
- [ ] Test backup restoration process
- [ ] Review and configure role-based permissions
- [ ] Set up monitoring for certificate expiration
- [ ] Configure message retention based on business needs (not HIPAA-mandated)

## Compliance Verification

Run the HIPAA compliance checker:

```bash
./bin/hl7db --check-compliance
```

This will verify:
- Encryption is enabled
- TLS certificates are valid
- Backup system is configured
- Security monitoring is running
- All required directories have correct permissions

## Support and Documentation

For additional help:
- Review `/var/log/hl7db/hl7db.log` for server logs
- Check backup manifest: `/var/hl7db/backups/backup_manifest.txt`

## Legal Notice

While HL7DB implements comprehensive HIPAA Security Rule controls, achieving full HIPAA compliance requires:
1. Proper configuration and deployment
2. Regular security assessments
3. Staff training
4. Business Associate Agreements
5. Physical security controls
6. Administrative safeguards
7. Organizational policies and procedures

**Important:** HL7DB is a query system for HL7 messages, not an EMR. Message retention policies should be based on your organization's needs, not HIPAA requirements (which apply to medical records, not transient HL7 messages).

Consult with legal and compliance experts to ensure full compliance with HIPAA regulations.
