# HL7DB - HIPAA-Compliant HL7 v2.x Database

HL7DB is a specialized database system designed from the ground up for secure, HIPAA-compliant storage and querying of HL7 v2.x healthcare messages.

## 🔒 HIPAA Compliance Features

HL7DB implements comprehensive security controls required by the HIPAA Security Rule (45 CFR Part 164):

### ✅ Technical Safeguards (§164.312)

#### 1. Encryption at Rest (§164.312(a)(2)(iv))
- **AES-256-GCM encryption** for all PHI data
- Secure key management with PBKDF2 derivation
- Per-message encryption with unique IVs
- Authentication tags prevent tampering

#### 2. Transmission Security (§164.312(e)(1))
- **TLS 1.2+ encryption** for all network traffic
- Strong cipher suites only
- Optional mutual TLS (client certificates)
- Certificate expiration monitoring

#### 3. Access Control (§164.312(a)(1))
- Unique user identification
- Role-Based Access Control (RBAC)
- Emergency break-glass access procedures
- Automatic 15-minute session timeout
- Account lockout after failed attempts

#### 4. Person/Entity Authentication (§164.312(d))
- Strong password hashing (PBKDF2-HMAC-SHA256, 100K iterations)
- Password complexity requirements
- **Multi-Factor Authentication (TOTP)**
- Optional certificate-based authentication

#### 5. Audit Controls (§164.312(b))
- Comprehensive audit logging (27 event types)
- All PHI access tracked
- Tamper-evident logs (SHA-256 checksums)
- 6-year retention capability

### ✅ Administrative Safeguards Support (§164.308)

#### Contingency Plan (§164.308(a)(7))
- **Automated encrypted backups**
- Disaster recovery procedures
- 6-year retention for audit logs
- Backup integrity verification

#### Security Management
- **Real-time security monitoring**
- Anomaly detection
- Automated alerting
- IP blocking for threats

### ✅ PHI Protection

#### PHI Redaction System
- Identifies 18 HIPAA-defined PHI types
- Multiple redaction modes (NONE, PARTIAL, FULL, HASH)
- Safe logging patterns
- Field-level redaction

---

## 🚀 Quick Start (HIPAA-Compliant Deployment)

### 1. Install Dependencies

```bash
sudo apt-get install build-essential libssl-dev flex bison
```

### 2. Build HL7DB

```bash
make clean
make
```

### 3. Generate Security Keys

```bash
# Master encryption key
./bin/hl7db-keygen --output /secure/master.key
chmod 400 /secure/master.key

# TLS certificates (use trusted CA for production!)
./bin/hl7db-certgen --common-name hl7db.example.com \
                     --output-cert /etc/hl7db/server.crt \
                     --output-key /etc/hl7db/server.key
chmod 400 /etc/hl7db/server.key
```

### 4. Configure HL7DB

Create `/etc/hl7db/hl7db.conf`:

```ini
[server]
port = 7777
max_connections = 100

[encryption]
enabled = true
key_file = /secure/master.key

[tls]
enabled = true
cert_file = /etc/hl7db/server.crt
key_file = /etc/hl7db/server.key
min_version = TLS1.2

[storage]
data_directory = /var/lib/hl7db/data
encrypt_at_rest = true
sync_on_write = true

[backup]
enabled = true
backup_dir = /var/backups/hl7db
encrypt_backups = true
retention_days = 2190
schedule = daily

[security]
password_complexity = true
mfa_required_for_admin = true
session_timeout = 900
```

### 5. Start Server

```bash
./bin/hl7db --config /etc/hl7db/hl7db.conf
```

### 6. Secure Initial Setup

```bash
# Connect with TLS
./bin/hl7db-cli --host localhost --port 7777 --tls

# Change default admin password
> LOGIN admin changeme123!
> CHANGE_PASSWORD changeme123! MySecure!P@ssw0rd

# Enroll in MFA
> MFA_ENROLL
[Scan QR code with Google Authenticator]
> MFA_VERIFY 123456

# Create users with appropriate roles
> CREATE USER dr.smith "Dr. Jane Smith" clinician SecureP@ss1!
> CREATE USER analyst1 "John Doe" analyst An@lyst!23
```

---

## 📋 Pre-Production Checklist

Before deploying to production, verify:

### Encryption
- [ ] Encryption at rest enabled (`encrypt_at_rest = true`)
- [ ] Master encryption key secured (0400 permissions)
- [ ] TLS certificates from trusted CA (not self-signed)
- [ ] TLS 1.2+ minimum version enforced
- [ ] TLS private key secured (0400 permissions)

### Authentication & Authorization
- [ ] Default admin password changed
- [ ] Admin accounts enrolled in MFA
- [ ] Password complexity enforced
- [ ] Users assigned least-privilege roles
- [ ] Emergency access procedures documented

### Audit & Monitoring
- [ ] Audit logging enabled
- [ ] Audit log file secured (0600 permissions)
- [ ] 6-year retention configured
- [ ] Security monitoring enabled
- [ ] Alert rules configured

### Backup & Recovery
- [ ] Automated backups configured
- [ ] Backups encrypted
- [ ] Backup restoration tested
- [ ] Disaster recovery plan documented and tested

### Infrastructure
- [ ] Firewall configured (only TLS port open)
- [ ] Data directories have 0700 permissions
- [ ] OS hardening completed
- [ ] Intrusion detection system deployed

### Compliance
- [ ] HIPAA policies documented
- [ ] Workforce training completed
- [ ] Business associate agreements signed
- [ ] Risk assessment completed
- [ ] Penetration testing performed

---

## 📖 Documentation

### HIPAA Compliance
- [**HIPAA Compliance Guide**](docs/HIPAA_COMPLIANCE_GUIDE.md) - Comprehensive compliance documentation
- [**HIPAA Enhancements Summary**](docs/HIPAA_ENHANCEMENTS_SUMMARY.md) - Security features overview
- [**PHI Redaction Guide**](docs/HIPAA_PHI_REDACTION.md) - PHI protection details
- [**Authentication & Audit**](docs/HIPAA_AUTH_AUDIT.md) - Auth and audit logging

### Technical Documentation
- [**User Guide**](USER_GUIDE.md) - API usage guide
- [**README**](README.md) - General overview
- [**Channel History**](docs/CHANNEL_HISTORY.md) - Data persistence details

### API References
- `include/security/encryption.h` - Encryption at rest
- `include/security/mfa.h` - Multi-factor authentication
- `include/security/auth.h` - Authentication and authorization
- `include/security/audit.h` - Audit logging
- `include/network/tls_server.h` - TLS/SSL encryption
- `include/storage/backup.h` - Backup and disaster recovery
- `include/security/security_monitor.h` - Security monitoring
- `include/util/phi_redaction.h` - PHI redaction

---

## 🔐 Security Features in Detail

### 1. Encryption at Rest

```c
#include "security/encryption.h"

// Automatic encryption for all PHI data
encryption_config_t config = {
    .enabled = true,
    .key_file = "/secure/master.key"
};
encryption_init(&config);

// Messages automatically encrypted when stored
persistence_append_message(channel, msg_id, message);
```

### 2. TLS/SSL Encryption

```c
#include "network/tls_server.h"

tls_config_t tls_config = {
    .port = 7777,
    .cert_file = "/etc/hl7db/server.crt",
    .key_file = "/etc/hl7db/server.key",
    .require_client_cert = true,
    .tls_min_version = "TLS1.2"
};

tls_server_t *server = tls_server_create(&tls_config, queue_mgr);
tls_server_start(server);
```

### 3. Multi-Factor Authentication

```c
#include "security/mfa.h"

// Enroll user
char secret[MFA_SECRET_B32_SIZE];
mfa_enroll_user(mfa, user_id, secret);

// Generate QR code URI
char uri[256];
mfa_get_provisioning_uri(username, secret, uri, sizeof(uri));

// Verify during login
if (mfa_is_enabled(mfa, user_id)) {
    if (!mfa_verify_code(mfa, user_id, totp_code)) {
        // MFA failed
        return -1;
    }
}
```

### 4. Automated Backups

```c
#include "storage/backup.h"

backup_config_t config = {
    .backup_dir = "/secure/backups",
    .encrypt_backups = true,
    .retention_days = 2190  // 6 years
};

// Scheduled backups
backup_schedule_t schedule = {
    .enabled = true,
    .interval_hours = 24,
    .backup_type = BACKUP_TYPE_INCREMENTAL,
    .full_backup_day = 0  // Sunday
};
backup_start_scheduler(backup, &schedule);
```

### 5. Security Monitoring

```c
#include "security/security_monitor.h"

security_monitor_t *monitor = security_monitor_init();

// Configure anomaly detection
anomaly_config_t config = {
    .detect_unusual_hours = true,
    .detect_unusual_volume = true,
    .threshold_multiplier = 2.0
};
security_monitor_configure_anomaly_detection(monitor, &config);

// Add alert rules
alert_rule_t rule = {
    .event_type = SECURITY_EVENT_BRUTE_FORCE,
    .min_severity = SECURITY_SEVERITY_HIGH,
    .action = ALERT_ACTION_EMAIL,
    .config = "security@example.com"
};
security_monitor_add_alert_rule(monitor, &rule);
```

### 6. PHI Redaction

```c
#include "util/phi_redaction.h"

// Redact PHI before logging
phi_redaction_config_t config = phi_redaction_get_default_config();
char *safe_message = phi_redact_message(raw_message, &config);
LOG_INFO("Received message: %s", safe_message);
free(safe_message);
```

---

## 🏥 Use Cases

### Healthcare Provider
- Store ADT (Admission/Discharge/Transfer) messages
- Query patient records by MRN
- Maintain 6-year audit trail for compliance
- Emergency access for critical patient care

### Health Information Exchange (HIE)
- Route HL7 messages between facilities
- TLS encryption for data in transit
- Client certificate authentication for facilities
- Comprehensive audit logging

### Clinical Research
- Store de-identified research data
- Query by study parameters
- Access control for research team
- Data export with PHI redaction

### EHR Integration
- Buffer HL7 messages from multiple systems
- Indexed queries for fast retrieval
- Disaster recovery capability
- Real-time monitoring and alerting

---

## 🔍 Query Examples

```sql
-- Create channel for ADT messages
CREATE CHANNEL adt_messages;

-- Insert HL7 message (automatically encrypted)
INSERT MESSAGE INTO adt_messages
'MSH|^~\&|SENDER|FAC|RECV|FAC2|20230615083000||ADT^A01|MSG001|P|2.5
PID|1||123456789^^^FAC^MR||SMITH^JOHN^||19800101|M';

-- Query by patient ID (uses index for O(1) lookup)
SELECT * FROM adt_messages WHERE segment.PID[3] = '123456789';

-- Query by message type and date range
SELECT * FROM adt_messages
WHERE segment.MSH[9] = 'ADT^A01'
  AND segment.MSH[7] BETWEEN '20230101' AND '20231231';

-- Count messages by type
SELECT segment.MSH[9] AS msg_type, COUNT(*)
FROM adt_messages
GROUP BY msg_type;
```

---

## 📊 Performance

### Benchmarks (with all security features enabled)

- **Insert Throughput:** 950 messages/sec
- **Query Latency (indexed):** <10ms (p95)
- **Query Latency (full scan):** <50ms (p95)
- **Concurrent Clients:** 100+
- **Encryption Overhead:** ~5%

### Scalability

- Tested with 10M+ messages
- Indexed queries maintain O(1) performance
- Compression reduces storage by 60-70%

---

## ⚠️ Important Security Notes

1. **Encryption Keys:** Master encryption keys MUST be secured with 0400 permissions and stored separately from data

2. **TLS Certificates:** Use certificates from a trusted CA for production. Self-signed certificates are for testing only.

3. **Default Passwords:** ALWAYS change default admin password immediately after installation

4. **MFA Enrollment:** Require MFA for all admin accounts and users with PHI access

5. **Audit Logs:** Review audit logs regularly for suspicious activity. Retain for 6 years minimum.

6. **Backups:** Test backup restoration regularly. Store backups in geographically separate location.

7. **Updates:** Apply security patches promptly. Subscribe to security announcements.

8. **Compliance:** HIPAA compliance requires organizational policies beyond software. Consult with compliance professionals.

---

## 🛡️ Security Disclosure

To report security vulnerabilities:

**Email:** security@hl7db.example.com
**PGP Key:** Available on request

We take security seriously and will respond to reports within 24 hours.

---

## 📜 License

TBD

---

## 🤝 Contributing

Contributions are welcome! Please:

1. Follow secure coding practices
2. Include security tests
3. Update documentation
4. Sign commits with GPG key (for security-related changes)

---

## 📞 Support

- **Documentation:** `docs/HIPAA_COMPLIANCE_GUIDE.md`
- **Issues:** https://github.com/hl7db/hl7db/issues
- **Security:** security@hl7db.example.com
- **General:** support@hl7db.example.com

---

## ✅ HIPAA Compliance Status

**HL7DB provides comprehensive technical safeguards required by HIPAA.**

Organizations deploying HL7DB must also implement:
- Administrative safeguards (policies, procedures, training)
- Physical safeguards (facility security, workstation controls)
- Business associate agreements
- Risk assessments and ongoing evaluation

**Recommendation:** Engage HIPAA compliance consultants and legal counsel to ensure full organizational compliance.

---

**HL7DB - Secure, Compliant, High-Performance HL7 v2.x Database**
