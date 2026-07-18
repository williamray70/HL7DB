# HIPAA Compliance Guide for HL7DB

## Executive Summary

HL7DB is designed to be a HIPAA-compliant database system for storing, querying, and managing HL7 v2.x healthcare messages containing Protected Health Information (PHI). This guide documents how HL7DB implements the required HIPAA Security Rule safeguards.

**Compliance Status:** HL7DB provides comprehensive technical safeguards and security controls required for HIPAA compliance. Organizations must supplement these with appropriate administrative and physical safeguards as outlined in this guide.

**Last Updated:** 2025-07-16

---

## Table of Contents

1. [HIPAA Requirements Overview](#hipaa-requirements-overview)
2. [Technical Safeguards Implementation](#technical-safeguards-implementation)
3. [Administrative Safeguards Guidance](#administrative-safeguards-guidance)
4. [Physical Safeguards Guidance](#physical-safeguards-guidance)
5. [Security Features](#security-features)
6. [Deployment Checklist](#deployment-checklist)
7. [Ongoing Compliance Requirements](#ongoing-compliance-requirements)
8. [Incident Response](#incident-response)
9. [Business Associate Agreements](#business-associate-agreements)

---

## HIPAA Requirements Overview

The HIPAA Security Rule (45 CFR Part 164, Subpart C) requires covered entities to implement:

### Administrative Safeguards (45 CFR §164.308)
- Security Management Process
- Workforce Security
- Information Access Management
- Security Awareness and Training
- Security Incident Procedures
- Contingency Plan
- Evaluation
- Business Associate Contracts

### Physical Safeguards (45 CFR §164.310)
- Facility Access Controls
- Workstation Use
- Workstation Security
- Device and Media Controls

### Technical Safeguards (45 CFR §164.312)
- Access Control
- Audit Controls
- Integrity
- Person or Entity Authentication
- Transmission Security

---

## Technical Safeguards Implementation

HL7DB implements comprehensive technical safeguards as required by 45 CFR §164.312.

### 1. Access Control (§164.312(a)(1))

**Implementation:**
- ✅ **Unique User Identification (§164.312(a)(2)(i))**: Each user has a unique username and user ID
- ✅ **Emergency Access Procedure (§164.312(a)(2)(ii))**: Break-glass emergency access with mandatory justification logging
- ✅ **Automatic Logoff (§164.312(a)(2)(iii))**: Sessions automatically expire after 15 minutes of inactivity
- ✅ **Encryption and Decryption (§164.312(a)(2)(iv))**: AES-256-GCM encryption for data at rest

**Configuration:**
```c
// See: include/security/auth.h
// Sessions timeout after SESSION_TIMEOUT_SECONDS (900 = 15 minutes)
```

**Files:**
- `include/security/auth.h`, `src/security/auth.c` - Authentication system
- `include/security/encryption.h`, `src/security/encryption.c` - Encryption at rest

---

### 2. Audit Controls (§164.312(b))

**Requirement:** "Implement hardware, software, and/or procedural mechanisms that record and examine activity in information systems that contain or use electronic protected health information (ePHI)."

**Implementation:**
- ✅ Comprehensive audit logging of all PHI access
- ✅ 27 distinct audit event types
- ✅ Tamper-evident logging (SHA-256 checksums, event chaining)
- ✅ JSON-formatted logs for analysis
- ✅ 6-year retention capability (HIPAA requirement)

**Audit Events Logged:**
- All authentication events (login, logout, failures)
- All PHI access (read, write, delete)
- Query execution with redacted PHI
- User account changes
- Permission changes
- Emergency access usage
- System configuration changes
- Access denied events

**Files:**
- `include/security/audit.h`, `src/security/audit.c` - Audit system
- `docs/HIPAA_AUTH_AUDIT.md` - Detailed audit documentation

**Audit Log Format:**
```json
{
  "event_id": 123,
  "timestamp": 1626450000,
  "type": "PHI_READ",
  "user_id": 5,
  "username": "dr.smith",
  "client_ip": "192.168.1.100",
  "resource": "adt_messages",
  "phi_accessed": true,
  "phi_record_count": 10,
  "checksum": "a3f2b1c4..."
}
```

---

### 3. Integrity (§164.312(c)(1))

**Requirement:** "Implement policies and procedures to protect electronic protected health information from improper alteration or destruction."

**Implementation:**
- ✅ Append-only message storage (messages are immutable)
- ✅ Authentication tags on encrypted data (prevents tampering)
- ✅ Audit log checksums (tamper detection)
- ✅ Atomic file operations for data integrity
- ✅ fsync support for durability

**Files:**
- `include/storage/persistence.h`, `src/storage/persistence.c` - Persistent storage

---

### 4. Person or Entity Authentication (§164.312(d))

**Requirement:** "Implement procedures to verify that a person or entity seeking access to electronic protected health information is the one claimed."

**Implementation:**
- ✅ Strong password authentication (PBKDF2-HMAC-SHA256, 100,000 iterations)
- ✅ Password complexity requirements (8+ chars, upper, lower, digit, special)
- ✅ Account lockout after 5 failed attempts (15-minute lockout)
- ✅ Secure session tokens (32 bytes cryptographically random)
- ✅ Multi-Factor Authentication (MFA/TOTP) support
- ✅ Optional client certificate authentication (TLS)

**Password Policy:**
- Minimum 8 characters
- At least one uppercase letter
- At least one lowercase letter
- At least one digit
- At least one special character
- Maximum 128 characters

**MFA Configuration:**
```c
// See: include/security/mfa.h
// TOTP with 6-digit codes, 30-second time step
```

**Files:**
- `include/security/auth.h`, `src/security/auth.c` - Authentication
- `include/security/mfa.h` - Multi-factor authentication

---

### 5. Transmission Security (§164.312(e)(1))

**Requirement:** "Implement technical security measures to guard against unauthorized access to electronic protected health information that is being transmitted over an electronic communications network."

**Implementation:**
- ✅ TLS 1.2+ encryption for all network communications
- ✅ Strong cipher suites only (no weak ciphers)
- ✅ Optional client certificate authentication
- ✅ Certificate expiration monitoring

**TLS Configuration:**
```c
// See: include/network/tls_server.h
tls_config_t config = {
    .port = 7777,
    .cert_file = "/path/to/server.crt",
    .key_file = "/path/to/server.key",
    .ca_file = "/path/to/ca.crt",
    .require_client_cert = true,  // For maximum security
    .tls_min_version = "TLS1.2",
    .cipher_list = NULL  // Uses secure defaults
};
```

**Supported TLS Versions:**
- TLS 1.2 (minimum)
- TLS 1.3 (recommended)

**Files:**
- `include/network/tls_server.h` - TLS server implementation

---

## Administrative Safeguards Guidance

HL7DB provides technical controls, but organizations must implement administrative safeguards per 45 CFR §164.308.

### 1. Security Management Process (§164.308(a)(1))

**Risk Analysis (§164.308(a)(1)(ii)(A))** - Required
- Conduct annual risk assessments
- Document identified risks to ePHI
- Evaluate likelihood and impact of threats
- Review HL7DB security controls

**Risk Management (§164.308(a)(1)(ii)(B))** - Required
- Implement security measures commensurate with risk
- Document risk mitigation strategies
- Review and update security controls annually

**Sanction Policy (§164.308(a)(1)(ii)(C))** - Required
- Establish sanctions for security policy violations
- Document disciplinary actions
- Include in employee handbook

**Information System Activity Review (§164.308(a)(1)(ii)(D))** - Required
- Regularly review HL7DB audit logs
- Investigate anomalies and suspicious activities
- Use security monitoring features (see `include/security/security_monitor.h`)

**Recommended Review Schedule:**
- Daily: Critical security events
- Weekly: Access patterns and failed authentication
- Monthly: Full audit log review
- Quarterly: Security posture assessment

---

### 2. Assigned Security Responsibility (§164.308(a)(2)) - Required

- Designate a Security Official responsible for HIPAA compliance
- Document security responsibilities
- Provide security official with authority and resources

---

### 3. Workforce Security (§164.308(a)(3))

**Authorization and/or Supervision (§164.308(a)(3)(ii)(A))** - Addressable
- Implement role-based access control (RBAC)
- Use HL7DB predefined roles: readonly, clinician, analyst, integration, admin
- Document access authorization procedures

**Workforce Clearance Procedure (§164.308(a)(3)(ii)(B))** - Addressable
- Perform background checks where appropriate
- Verify workforce member credentials
- Document clearance procedures

**Termination Procedures (§164.308(a)(3)(ii)(C))** - Addressable
- Immediately disable terminated user accounts
- Review terminated user's PHI access in audit logs
- Retrieve all access credentials and devices

**HL7DB Integration:**
```bash
# Disable user account immediately upon termination
auth_disable_user(auth_system, user_id);

# Review their PHI access
audit_query_user_activity(audit_system, user_id, start_time, end_time);
```

---

### 4. Information Access Management (§164.308(a)(4))

**Access Authorization (§164.308(a)(4)(ii)(C))** - Addressable
- Grant minimum necessary access using HL7DB roles
- Document access approval process
- Review access periodically

**HL7DB Roles and Permissions:**

| Role | Permissions | Use Case |
|------|------------|----------|
| readonly | View non-PHI data | Reports, analytics |
| clinician | Read/write PHI | Healthcare providers |
| analyst | Read PHI, no write | Research, quality improvement |
| integration | Read/write PHI, channel management | System integrations |
| admin | Full administrative access | System administrators |
| emergency | Break-glass emergency access | Emergency situations only |

---

### 5. Security Awareness and Training (§164.308(a)(5)) - Required

**Security Reminders (§164.308(a)(5)(ii)(A))** - Addressable
- Provide ongoing security awareness training
- Distribute security policy updates
- Send periodic reminders about:
  - Password security
  - PHI handling procedures
  - Incident reporting
  - Emergency access procedures

**Protection from Malicious Software (§164.308(a)(5)(ii)(B))** - Addressable
- Implement endpoint protection on workstations
- Keep systems updated with security patches
- Use application whitelisting where possible

**Log-in Monitoring (§164.308(a)(5)(ii)(C))** - Addressable
- Monitor HL7DB audit logs for suspicious login activity
- Use security monitoring features (see `include/security/security_monitor.h`)
- Alert on:
  - Multiple failed login attempts
  - Unusual login times
  - Login from unexpected locations

**Password Management (§164.308(a)(5)(ii)(D))** - Addressable
- Enforce HL7DB password complexity requirements
- Implement password expiration (recommended 90 days)
- Prevent password reuse (recommended last 5 passwords)
- Prohibit password sharing

---

### 6. Security Incident Procedures (§164.308(a)(6)) - Required

**Response and Reporting (§164.308(a)(6)(ii))** - Required

**Incident Response Plan:**

1. **Identification**
   - Monitor HL7DB security alerts
   - Review audit logs for suspicious activity
   - Investigate user reports

2. **Containment**
   - Lock compromised accounts: `auth_lock_user()`
   - Block malicious IP addresses: `security_monitor_block_ip()`
   - Isolate affected systems

3. **Eradication**
   - Remove unauthorized access
   - Patch vulnerabilities
   - Update security controls

4. **Recovery**
   - Restore from backups if needed
   - Verify system integrity
   - Resume normal operations

5. **Documentation**
   - Document all incidents
   - Track response actions
   - Maintain incident log

6. **Notification**
   - Notify affected individuals (if required)
   - Report to HHS if breach affects 500+ individuals
   - Notify business associates if applicable

**Use HL7DB Features:**
```c
// Generate incident report from audit logs
audit_export_range(audit_system, incident_start, incident_end,
                   "/secure/incident_123_audit.json");

// Block attacker IP
security_monitor_block_ip(monitor, "192.168.1.50", 0,
                          "Incident #123 - Brute force attack");
```

---

### 7. Contingency Plan (§164.308(a)(7)) - Required

**Data Backup Plan (§164.308(a)(7)(ii)(A))** - Required
- Implement automated backups using HL7DB backup system
- Store backups in geographically separate location
- Encrypt all backups (AES-256-GCM)
- Test backups regularly (monthly recommended)

**Disaster Recovery Plan (§164.308(a)(7)(ii)(B))** - Required
- Document procedures to restore HL7DB after disaster
- Maintain Recovery Time Objective (RTO) and Recovery Point Objective (RPO)
- Test disaster recovery annually

**Emergency Mode Operation Plan (§164.308(a)(7)(ii)(C))** - Required
- Document procedures for emergency access to PHI
- Use HL7DB emergency access features
- Require justification for all emergency access

**Backup Configuration:**
```c
// See: include/storage/backup.h
backup_config_t backup_config = {
    .backup_dir = "/secure/backups",
    .data_dir = "/var/lib/hl7db/data",
    .encrypt_backups = true,
    .retention_days = 2190,  // 6 years for audit logs
    .default_type = BACKUP_TYPE_INCREMENTAL
};

// Schedule daily backups
backup_schedule_t schedule = {
    .enabled = true,
    .interval_hours = 24,
    .backup_type = BACKUP_TYPE_INCREMENTAL,
    .full_backup_day = 0  // Sunday
};
```

**Retention Policy:**
- PHI data: Per organizational policy
- Audit logs: Minimum 6 years (HIPAA requirement)
- Backups: 6 years for audit logs
- Incident reports: 6 years

---

### 8. Evaluation (§164.308(a)(8)) - Required

**Requirement:** "Perform a periodic technical and nontechnical evaluation...in response to environmental or operational changes..."

**Recommended Evaluation Schedule:**
- Annually: Complete security evaluation
- After significant changes: System upgrades, architecture changes
- After security incidents: Post-incident review

**Evaluation Checklist:**
- [ ] Review HL7DB security features and configuration
- [ ] Audit user accounts and permissions
- [ ] Review audit logs for anomalies
- [ ] Test backup and disaster recovery procedures
- [ ] Verify encryption is enabled (data at rest and in transit)
- [ ] Review password policies and MFA enrollment
- [ ] Test security monitoring and alerting
- [ ] Update risk assessment
- [ ] Review and update security policies

---

### 9. Business Associate Agreements (§164.308(b))

If your organization is a covered entity working with business associates (BAs) who access HL7DB:

**Required BAA Provisions:**
- BA must comply with HIPAA Security Rule
- BA must report security incidents to covered entity
- BA must ensure subcontractors are compliant
- BA must return or destroy PHI at termination
- Covered entity may terminate for material breach

**HL7DB Access for Business Associates:**
- Create dedicated user accounts with appropriate roles
- Use client certificate authentication for system integrations
- Audit all BA access to PHI
- Review BA activity quarterly

---

## Physical Safeguards Guidance

Physical safeguards (45 CFR §164.310) are deployment-specific. Organizations must:

### 1. Facility Access Controls (§164.310(a)(1))

- Limit physical access to servers running HL7DB
- Maintain visitor logs
- Use badge access systems
- Implement video surveillance where appropriate

### 2. Workstation Use (§164.310(b)) - Required

- Establish policies for workstation use
- Prohibit leaving workstations unattended while logged in
- Use screen locks with timeout
- Position screens away from public view

### 3. Workstation Security (§164.310(c)) - Required

- Implement physical security for workstations
- Use cable locks for portable devices
- Encrypt workstation hard drives
- Disable unauthorized USB devices

### 4. Device and Media Controls (§164.310(d)(1))

**Disposal (§164.310(d)(2)(i))** - Required
- Securely wipe or destroy media containing PHI
- Use certified data destruction services
- Maintain disposal logs

**Media Re-use (§164.310(d)(2)(ii))** - Required
- Sanitize media before re-use
- Verify complete data removal

**Accountability (§164.310(d)(2)(iii))** - Addressable
- Track hardware containing HL7DB data
- Maintain inventory of storage media
- Document media movements

---

## Security Features

### PHI Redaction

HL7DB automatically redacts PHI from logs and non-authorized outputs.

**Features:**
- Identifies 18 HIPAA-defined PHI types
- Four redaction modes: NONE, PARTIAL, FULL, HASH
- Field-level redaction for HL7 segments

**Usage:**
```c
#include "util/phi_redaction.h"

// For logs - full redaction
phi_redaction_config_t config = phi_redaction_get_default_config();
char *safe_message = phi_redact_message(raw_message, &config);
LOG_INFO("Received message: %s", safe_message);
free(safe_message);
```

**Documentation:** `docs/HIPAA_PHI_REDACTION.md`

---

### Encryption at Rest

All PHI data is encrypted using AES-256-GCM before storage.

**Setup:**
```bash
# Generate master encryption key
./bin/hl7db-keygen --output /secure/master.key
chmod 400 /secure/master.key

# Configure encryption in hl7db.conf
[encryption]
enabled = true
key_file = /secure/master.key
```

**Key Management Best Practices:**
- Store master key on separate encrypted volume
- Use Hardware Security Module (HSM) for production
- Rotate keys annually
- Maintain secure key backups

**Documentation:** `include/security/encryption.h`

---

### TLS/SSL Encryption

All network traffic is encrypted using TLS 1.2+.

**Setup:**
```bash
# For production, obtain certificates from trusted CA
# For testing, generate self-signed certificates:
./bin/hl7db-certgen --output-cert server.crt --output-key server.key \
                     --common-name hl7db.example.com --days 365

# Configure TLS in hl7db.conf
[tls]
enabled = true
cert_file = /path/to/server.crt
key_file = /path/to/server.key
min_version = TLS1.2
require_client_cert = true  # For maximum security
```

**Client Connection:**
```bash
# Clients must use TLS
hl7db-cli --host hl7db.example.com --port 7777 --tls \
          --ca-cert /path/to/ca.crt \
          --client-cert /path/to/client.crt \
          --client-key /path/to/client.key
```

**Documentation:** `include/network/tls_server.h`

---

### Multi-Factor Authentication

MFA adds an extra layer of security using TOTP (Time-based One-Time Passwords).

**Enrollment:**
```c
// Enroll user in MFA
char totp_secret[MFA_SECRET_B32_SIZE];
mfa_enroll_user(mfa_system, user_id, totp_secret);

// Generate QR code URI for Google Authenticator, Authy, etc.
char uri[256];
mfa_get_provisioning_uri(username, totp_secret, uri, sizeof(uri));
// Display URI as QR code to user
```

**Login with MFA:**
```c
// Standard login
char *session = auth_login(auth, username, password, client_ip, "App/1.0");

// If MFA enabled, require TOTP code
if (mfa_is_enabled(mfa, user_id)) {
    char totp_code[7];
    // Prompt user for TOTP code

    if (!mfa_verify_code(mfa, user_id, totp_code)) {
        auth_logout(auth, session);
        // Login failed - invalid MFA code
    }
}
```

**Backup Codes:**
```c
// Generate backup codes for account recovery
char backup_codes[10][11];
mfa_generate_backup_codes(mfa, user_id, backup_codes);
// Securely provide these to user
```

**Documentation:** `include/security/mfa.h`

---

### Backup and Disaster Recovery

Automated backup system with encryption and retention management.

**Configuration:**
```c
backup_config_t config = backup_get_default_config();
config.backup_dir = "/secure/backups";
config.data_dir = "/var/lib/hl7db/data";
config.encrypt_backups = true;
config.retention_days = 2190;  // 6 years

backup_system_t *backup = backup_system_init(&config);
```

**Automated Backups:**
```c
// Schedule daily incremental, weekly full
backup_schedule_t schedule = {
    .enabled = true,
    .interval_hours = 24,
    .backup_type = BACKUP_TYPE_INCREMENTAL,
    .full_backup_day = 0  // Sunday
};
backup_start_scheduler(backup, &schedule);
```

**Disaster Recovery Testing:**
```c
// Test disaster recovery plan (doesn't affect production)
backup_test_disaster_recovery(backup, "/tmp/dr_test");
```

**Documentation:** `include/storage/backup.h`

---

### Security Monitoring and Alerting

Real-time monitoring for security threats and anomalies.

**Setup:**
```c
security_monitor_t *monitor = security_monitor_init();

// Configure anomaly detection
anomaly_config_t anomaly_config = {
    .detect_unusual_hours = true,
    .detect_unusual_volume = true,
    .threshold_multiplier = 2.0
};
security_monitor_configure_anomaly_detection(monitor, &anomaly_config);
```

**Alert Rules:**
```c
// Alert on brute force attacks
alert_rule_t rule = {
    .event_type = SECURITY_EVENT_BRUTE_FORCE,
    .min_severity = SECURITY_SEVERITY_HIGH,
    .action = ALERT_ACTION_EMAIL,
    .config = "security@example.com",
    .enabled = true
};
security_monitor_add_alert_rule(monitor, &rule);
```

**IP Blocking:**
```c
// Automatically block IPs after brute force attempts
security_monitor_block_ip(monitor, "192.168.1.100", 3600,
                          "Brute force attack detected");
```

**Documentation:** `include/security/security_monitor.h`

---

## Deployment Checklist

Use this checklist before deploying HL7DB to production:

### Pre-Deployment

- [ ] **Risk Assessment Completed**
  - [ ] Documented potential security risks
  - [ ] Evaluated likelihood and impact
  - [ ] Identified mitigation strategies

- [ ] **Policies and Procedures**
  - [ ] Security policies documented
  - [ ] Workforce training completed
  - [ ] Incident response plan in place
  - [ ] Contingency plan documented

### HL7DB Configuration

- [ ] **Encryption**
  - [ ] Master encryption key generated and secured
  - [ ] Encryption at rest enabled (`encrypt_at_rest = true`)
  - [ ] Key file permissions set to 0400
  - [ ] TLS certificates obtained from trusted CA (not self-signed)
  - [ ] TLS 1.2+ required (`tls_min_version = TLS1.2`)

- [ ] **Authentication**
  - [ ] Default admin password changed
  - [ ] Password complexity enforcement enabled
  - [ ] MFA enrollment required for admin accounts
  - [ ] Session timeout configured (15 minutes)
  - [ ] Account lockout enabled (5 failed attempts)

- [ ] **Authorization**
  - [ ] Users assigned least-privilege roles
  - [ ] Emergency access procedures documented
  - [ ] Role-based access control reviewed

- [ ] **Audit Logging**
  - [ ] Audit log path configured
  - [ ] Log file permissions set to 0600
  - [ ] 6-year retention enabled for audit logs
  - [ ] Log review schedule established

- [ ] **Backup**
  - [ ] Backup directory configured
  - [ ] Encrypted backups enabled
  - [ ] Automated backup schedule configured
  - [ ] Backup restoration tested
  - [ ] Off-site backup storage configured

### Infrastructure

- [ ] **Network Security**
  - [ ] Firewall configured (allow only port 7777 with TLS)
  - [ ] Intrusion detection system deployed
  - [ ] Network segmentation implemented
  - [ ] VPN required for remote access

- [ ] **Server Security**
  - [ ] Operating system hardened
  - [ ] Automatic security updates enabled
  - [ ] Unnecessary services disabled
  - [ ] File permissions restricted (0700 for data directories)
  - [ ] SELinux/AppArmor enabled

- [ ] **Physical Security**
  - [ ] Server in locked facility
  - [ ] Access controls in place
  - [ ] Visitor logging implemented
  - [ ] Environmental controls (cooling, power)

### Testing

- [ ] **Security Testing**
  - [ ] Penetration testing completed
  - [ ] Vulnerability scan passed
  - [ ] Security code review completed

- [ ] **Functional Testing**
  - [ ] Authentication tested
  - [ ] Authorization tested
  - [ ] Encryption verified (at rest and in transit)
  - [ ] Audit logging verified
  - [ ] Backup/restore tested
  - [ ] Disaster recovery tested

### Documentation

- [ ] **System Documentation**
  - [ ] Architecture diagram
  - [ ] Network diagram
  - [ ] Configuration guide
  - [ ] Operations manual

- [ ] **Security Documentation**
  - [ ] Security policies
  - [ ] Incident response plan
  - [ ] Disaster recovery plan
  - [ ] Audit log review procedures
  - [ ] User access management procedures

### Legal and Compliance

- [ ] **Business Associate Agreements**
  - [ ] BAAs executed with all business associates
  - [ ] BAA provisions comply with HIPAA

- [ ] **Policies**
  - [ ] Privacy policy published
  - [ ] Security policy published
  - [ ] Sanctions policy established
  - [ ] Workforce training completed

---

## Ongoing Compliance Requirements

### Daily

- [ ] Review critical security alerts
- [ ] Monitor system health
- [ ] Verify backups completed successfully

### Weekly

- [ ] Review failed authentication attempts
- [ ] Review emergency access usage
- [ ] Check certificate expiration dates

### Monthly

- [ ] Full audit log review
- [ ] User access review
- [ ] Security patch review and installation
- [ ] Backup restoration test

### Quarterly

- [ ] Security posture assessment
- [ ] Business associate activity review
- [ ] Policy and procedure review
- [ ] Workforce security training

### Annually

- [ ] Complete security evaluation
- [ ] Risk assessment update
- [ ] Disaster recovery test
- [ ] Penetration testing
- [ ] Security policies review and update

---

## Incident Response

### Incident Types

**Security Incidents:**
- Unauthorized access to PHI
- Data breach or suspected breach
- Malware infection
- Denial of service attack
- Lost or stolen devices containing PHI

**Breach Notification Requirements:**

**Breaches Affecting 500+ Individuals:**
- Notify HHS within 60 days
- Notify affected individuals within 60 days
- Notify media

**Breaches Affecting <500 Individuals:**
- Notify affected individuals within 60 days
- Notify HHS annually

### Incident Response Procedure

1. **Detect and Report**
   - Monitor HL7DB security alerts
   - Investigate anomalies
   - Report incidents to Security Official

2. **Assess**
   - Determine scope of incident
   - Identify affected PHI
   - Count affected individuals
   - Review audit logs

3. **Contain**
   ```c
   // Lock compromised accounts
   auth_lock_user(auth, user_id);

   // Block malicious IPs
   security_monitor_block_ip(monitor, attacker_ip, 0, "Incident #123");

   // Export audit trail
   audit_export_range(audit, incident_start, incident_end,
                      "/secure/incident_123.json");
   ```

4. **Remediate**
   - Remove unauthorized access
   - Patch vulnerabilities
   - Reset compromised credentials
   - Restore from backups if needed

5. **Document**
   - Complete incident report form
   - Document timeline
   - Document actions taken
   - Document affected individuals

6. **Notify**
   - Determine if breach notification required
   - Notify affected individuals (if required)
   - Notify HHS (if required)
   - Notify business associates (if applicable)

7. **Review and Improve**
   - Conduct post-incident review
   - Update security controls
   - Revise policies/procedures
   - Additional workforce training if needed

---

## Business Associate Agreements

### When BAAs Are Required

- Vendors who access HL7DB containing PHI
- Cloud hosting providers
- IT support contractors
- Data backup services
- Security monitoring services

### BAA Required Provisions

1. **Permitted Uses**: BA may only use PHI for services specified
2. **Safeguards**: BA must implement appropriate safeguards
3. **Subcontractors**: BA must ensure subcontractor BAAs
4. **Reporting**: BA must report security incidents
5. **Access**: BA must provide access to PHI for individuals
6. **Accounting**: BA must track PHI disclosures
7. **Amendment**: BA must make amendments to PHI
8. **Return/Destroy**: BA must return or destroy PHI at termination
9. **Audit**: Covered entity may audit BA compliance
10. **Termination**: Covered entity may terminate for breach

---

## Support and Resources

### HL7DB Documentation

- `README.md` - General overview
- `USER_GUIDE.md` - API usage guide
- `docs/HIPAA_PHI_REDACTION.md` - PHI redaction details
- `docs/HIPAA_AUTH_AUDIT.md` - Authentication and audit details
- `docs/HIPAA_COMPLIANCE_GUIDE.md` - This document

### HIPAA Resources

- [HHS HIPAA Homepage](https://www.hhs.gov/hipaa/index.html)
- [HIPAA Security Rule](https://www.hhs.gov/hipaa/for-professionals/security/index.html)
- [NIST SP 800-66: HIPAA Security Rule Guide](https://csrc.nist.gov/publications/detail/sp/800-66/rev-2/draft)

### Reporting Issues

For security vulnerabilities, contact: security@hl7db.example.com

For general issues: https://github.com/hl7db/hl7db/issues

---

## Disclaimer

This guide provides information about how HL7DB implements technical safeguards required by the HIPAA Security Rule. However, HIPAA compliance requires organizational policies, procedures, and administrative safeguards beyond what any software system can provide.

Organizations deploying HL7DB are responsible for:
- Conducting risk assessments
- Implementing appropriate administrative and physical safeguards
- Establishing policies and procedures
- Training workforce members
- Executing business associate agreements
- Maintaining ongoing compliance

**Consult with legal counsel and compliance professionals to ensure full HIPAA compliance.**

---

**Document Version:** 1.0
**Last Updated:** 2025-07-16
**Next Review:** 2026-07-16
