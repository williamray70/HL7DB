# HIPAA PHI Redaction in HL7DB

## Overview

HL7DB implements Protected Health Information (PHI) redaction to ensure compliance with the **HIPAA Security Rule (45 CFR §164.312)** - Technical Safeguards.

This feature prevents PHI from being exposed in:
- Application logs
- Error messages
- Console output
- Debug traces
- Audit trails
- Non-secure contexts

## HIPAA Compliance

### Relevant Regulations

**45 CFR §164.312(a)(1) - Access Control**
> Implement technical policies and procedures that allow only authorized persons to access electronic protected health information (ePHI).

**45 CFR §164.312(b) - Audit Controls**
> Implement hardware, software, and/or procedural mechanisms that record and examine activity in information systems that contain or use ePHI.

**45 CFR §164.312(e)(1) - Transmission Security**
> Implement technical security measures to guard against unauthorized access to ePHI that is being transmitted over an electronic communications network.

### What is PHI?

Under HIPAA, PHI includes any of the following **18 identifiers**:

1. Names
2. Geographic subdivisions smaller than state
3. Dates (birth, admission, discharge, death, etc.)
4. Telephone numbers
5. Fax numbers
6. Email addresses
7. Social Security numbers
8. Medical record numbers
9. Health plan beneficiary numbers
10. Account numbers
11. Certificate/license numbers
12. Vehicle identifiers
13. Device identifiers
14. URLs
15. IP addresses
16. Biometric identifiers
17. Full-face photos
18. Any unique identifying number, characteristic, or code

### HL7DB PHI Protection

HL7DB automatically detects and redacts PHI from HL7 v2.x messages based on standard field definitions:

| HL7 Field | Description | PHI Type | Redacted? |
|-----------|-------------|----------|-----------|
| PID-3 | Patient ID (MRN) | Identifier #8 | ✓ Yes |
| PID-5 | Patient Name | Identifier #1 | ✓ Yes |
| PID-7 | Date of Birth | Identifier #3 | ✓ Yes |
| PID-11 | Patient Address | Identifier #2 | ✓ Yes |
| PID-13 | Phone Number | Identifier #4 | ✓ Yes |
| PID-14 | Business Phone | Identifier #4 | ✓ Yes |
| PID-18 | Account Number | Identifier #10 | ✓ Yes |
| PID-19 | SSN | Identifier #7 | ✓ Yes |
| PID-20 | Driver's License | Identifier #11 | ✓ Yes |
| NK1-2 | Next of Kin Name | Identifier #1 | ✓ Yes |
| NK1-4 | Next of Kin Address | Identifier #2 | ✓ Yes |
| NK1-5 | Next of Kin Phone | Identifier #4 | ✓ Yes |
| MSH-9 | Message Type | Not PHI | ✗ No |
| MSH-3,4,5,6 | Facility Codes | Not PHI | ✗ No |

## API Reference

### Header File

```c
#include "util/phi_redaction.h"
```

### Redaction Modes

```c
typedef enum {
    PHI_REDACT_NONE = 0,        /* No redaction (authorized contexts only) */
    PHI_REDACT_PARTIAL,         /* Partial: "John Doe" → "J*** D**" */
    PHI_REDACT_FULL,            /* Full: "John Doe" → "[REDACTED]" */
    PHI_REDACT_HASH             /* Hash: "John Doe" → "[PHI:a3f2b1c4]" */
} phi_redaction_mode_t;
```

### Configuration

```c
typedef struct {
    phi_redaction_mode_t mode;  /* Redaction mode */
    bool redact_names;          /* Redact patient/provider names */
    bool redact_ids;            /* Redact patient IDs (MRN, SSN) */
    bool redact_addresses;      /* Redact street addresses */
    bool redact_dates;          /* Redact dates (birth, admission, etc.) */
    bool redact_phone;          /* Redact phone numbers */
    bool redact_email;          /* Redact email addresses */
    bool redact_accounts;       /* Redact account numbers */
    bool allow_msg_type;        /* Allow message type (not PHI) */
    bool allow_facility;        /* Allow facility codes (not PHI) */
} phi_redaction_config_t;
```

### Getting Default Configurations

```c
/* HIPAA-compliant default (full redaction) */
phi_redaction_config_t config = phi_redaction_get_default_config();

/* Debug mode (partial redaction, shows dates) */
phi_redaction_config_t debug = phi_redaction_get_debug_config();

/* No redaction (authorized processing only) */
phi_redaction_config_t none = phi_redaction_get_none_config();
```

### Core Functions

#### Redact Individual Fields

```c
char *phi_redact_field(const char *field_id,
                       const char *value,
                       const phi_redaction_config_t *config);
```

**Example:**
```c
phi_redaction_config_t config = phi_redaction_get_default_config();

char *name = phi_redact_field("PID-5", "JONES^WILLIAM^A", &config);
// Returns: "[REDACTED]"

char *msg_type = phi_redact_field("MSH-9", "ADT^A01", &config);
// Returns: "ADT^A01" (not PHI, not redacted)

free(name);
free(msg_type);
```

#### Redact Entire Messages

```c
char *phi_redact_message(const char *raw_message,
                         const phi_redaction_config_t *config);
```

**Example:**
```c
const char *msg =
    "MSH|^~\\&|APP|FAC|RECV|FAC2|20230615083000||ADT^A01|MSG001|P|2.5\r"
    "PID|1||123456789^^^FAC^MR||SMITH^JOHN^||19800101|M";

phi_redaction_config_t config = phi_redaction_get_default_config();
char *redacted = phi_redact_message(msg, &config);

// Result:
// MSH|^~\&|APP|FAC|RECV|FAC2|20230615083000||ADT^A01|MSG001|P|2.5
// PID|1||[REDACTED]||[REDACTED]||[REDACTED]|M

free(redacted);
```

#### Print Messages with Redaction

```c
void hl7_message_print_redacted(const hl7_message_t *msg,
                                 int verbose,
                                 const phi_redaction_config_t *config);
```

**Example:**
```c
hl7_message_t *msg = hl7_parse(raw_message, NULL);

/* Safe for logs */
phi_redaction_config_t config = phi_redaction_get_default_config();
hl7_message_print_redacted(msg, 1, &config);

hl7_message_destroy(msg);
```

#### Print Query Results with Redaction

```c
void hl7sql_result_print_redacted(const hl7sql_result_t *result,
                                   const phi_redaction_config_t *config);
```

**Example:**
```c
hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM adt_messages");

/* Safe for logs */
phi_redaction_config_t config = phi_redaction_get_default_config();
hl7sql_result_print_redacted(result, &config);

hl7sql_result_destroy(result);
```

## Usage Patterns

### Pattern 1: Safe Logging

```c
#include "util/logger.h"
#include "util/phi_redaction.h"

void process_message(const char *raw_msg) {
    hl7_message_t *msg = hl7_parse(raw_msg, NULL);

    /* UNSAFE - PHI in logs! */
    // LOG_INFO("Received message: %s", raw_msg);

    /* SAFE - PHI redacted */
    phi_redaction_config_t config = phi_redaction_get_default_config();
    char *redacted = phi_redact_message(raw_msg, &config);
    LOG_INFO("Received message: %s", redacted);
    free(redacted);

    /* Process with full PHI (authorized) */
    const char *patient_id = hl7_message_get_field(msg, "PID-3");
    // ... authorized processing ...

    hl7_message_destroy(msg);
}
```

### Pattern 2: Audit Trail

```c
void log_audit_event(hl7_message_t *msg) {
    /* Extract non-PHI metadata */
    const char *msg_type = hl7_message_get_field(msg, "MSH-9");
    const char *msg_id = hl7_message_get_field(msg, "MSH-10");
    const char *facility = hl7_message_get_field(msg, "MSH-3");

    /* Redact patient ID for audit log */
    const char *patient_id = hl7_message_get_field(msg, "PID-3");
    phi_redaction_config_t config = phi_redaction_get_default_config();
    char *redacted_id = phi_redact_field("PID-3", patient_id, &config);

    /* Write to audit log - no PHI exposed */
    fprintf(audit_log, "timestamp=%ld facility=%s msg_type=%s msg_id=%s patient=%s\n",
            time(NULL), facility, msg_type, msg_id, redacted_id);

    free(redacted_id);
}
```

### Pattern 3: Error Reporting

```c
void handle_validation_error(hl7_message_t *msg, const char *error) {
    /* Use partial redaction for debugging while protecting PHI */
    phi_redaction_config_t config = phi_redaction_get_debug_config();

    /* Print redacted message for troubleshooting */
    fprintf(stderr, "Validation error: %s\n", error);
    fprintf(stderr, "Message (PHI partially redacted):\n");
    hl7_message_print_redacted(msg, 1, &config);
}
```

### Pattern 4: Different Contexts

```c
void display_message(hl7_message_t *msg, security_context_t *ctx) {
    if (ctx->authorized_for_phi) {
        /* Full PHI access for authorized users */
        phi_redaction_config_t config = phi_redaction_get_none_config();
        hl7_message_print_redacted(msg, 1, &config);
    } else if (ctx->debug_mode && ctx->secure_environment) {
        /* Partial redaction for debugging in secure environment */
        phi_redaction_config_t config = phi_redaction_get_debug_config();
        hl7_message_print_redacted(msg, 1, &config);
    } else {
        /* Full redaction for logs, non-secure output */
        phi_redaction_config_t config = phi_redaction_get_default_config();
        hl7_message_print_redacted(msg, 1, &config);
    }
}
```

## Security Best Practices

### DO ✓

1. **Always use redaction for logs**
   ```c
   phi_redaction_config_t config = phi_redaction_get_default_config();
   char *safe = phi_redact_message(msg, &config);
   LOG_INFO("Message: %s", safe);
   free(safe);
   ```

2. **Use partial redaction only in secure environments**
   ```c
   /* Only in secure, access-controlled debug sessions */
   if (is_secure_debug_session()) {
       phi_redaction_config_t config = phi_redaction_get_debug_config();
       hl7_message_print_redacted(msg, 1, &config);
   }
   ```

3. **Maintain audit trails without PHI**
   ```c
   /* Log events with metadata, not raw PHI */
   LOG_AUDIT("event=MESSAGE_RECEIVED msg_type=%s facility=%s",
             msg_type, facility);
   ```

4. **Use hash mode for correlation**
   ```c
   /* When you need to correlate messages without exposing PHI */
   phi_redaction_config_t config = phi_redaction_get_default_config();
   config.mode = PHI_REDACT_HASH;
   char *hash = phi_redact_field("PID-3", patient_id, &config);
   // Returns: "[PHI:a3f2b1c4]" - same hash for same ID
   free(hash);
   ```

### DON'T ✗

1. **Never log raw PHI**
   ```c
   /* BAD - PHI in logs! */
   LOG_INFO("Patient: %s", patient_name);

   /* GOOD - Redacted */
   char *safe = phi_redact_field("PID-5", patient_name, &config);
   LOG_INFO("Patient: %s", safe);
   free(safe);
   ```

2. **Never disable redaction in production logs**
   ```c
   /* BAD - No redaction in production! */
   #ifdef PRODUCTION
   phi_redaction_config_t config = phi_redaction_get_none_config();
   #endif
   ```

3. **Never print raw messages to console in multi-user environments**
   ```c
   /* BAD - PHI visible to anyone watching console */
   printf("Message: %s\n", raw_message);

   /* GOOD - Redacted output */
   hl7_message_print_redacted(msg, 1, &config);
   ```

4. **Never include PHI in error messages**
   ```c
   /* BAD - PHI in error message */
   return error("Invalid patient ID: %s", patient_id);

   /* GOOD - Generic error, log details separately with redaction */
   LOG_ERROR("Invalid patient ID (see secure audit log)");
   return error("Invalid patient ID format");
   ```

## Testing

Run the PHI redaction test suite:

```bash
gcc -Iinclude -Isrc test_phi_redaction.c -o bin/test_phi_redaction \
    <object_files> -lpthread

./bin/test_phi_redaction
```

The test suite validates:
- Field-level redaction for all modes
- Full message redaction
- Message printing with redaction
- Query result redaction
- HIPAA compliance scenarios

## Compliance Checklist

Use this checklist to ensure your HL7DB integration is HIPAA-compliant:

- [ ] All application logs use `phi_redaction` before logging HL7 messages
- [ ] Error messages never contain raw PHI
- [ ] Console output in production uses redacted printing
- [ ] Audit trails log metadata only, not raw PHI
- [ ] Debug/trace output uses partial or full redaction
- [ ] PHI access is limited to authorized code paths only
- [ ] No PHI in filenames, paths, or URLs
- [ ] Network transmission logs redact message content
- [ ] Database query logs don't expose PHI in WHERE clauses
- [ ] PHI is encrypted at rest and in transit (beyond redaction)

## Performance Considerations

### Redaction Overhead

- **Field-level redaction**: ~1-5 µs per field
- **Full message redaction**: ~10-50 µs per message (depends on size)
- **Negligible impact** on overall message processing throughput

### When to Redact

Redact as late as possible in the pipeline:

1. **Internal Processing**: Use full PHI (authorized context)
2. **Before Logging**: Apply redaction
3. **Before Display**: Apply redaction
4. **Before Network**: Encrypt + redact for logs

### Caching Considerations

Don't cache redacted messages - always redact fresh:

```c
/* BAD - Stale redacted copy */
static char *cached_redacted = NULL;

/* GOOD - Redact on demand */
char *redacted = phi_redact_message(msg, &config);
LOG_INFO("%s", redacted);
free(redacted);
```

## Future Enhancements

Planned improvements:

1. **Configurable PHI field mapping** - Custom field definitions
2. **Regular expression patterns** - Detect PHI in unstructured text
3. **Redaction policies** - Role-based redaction levels
4. **Audit log integration** - Track who accessed PHI
5. **Tokenization** - Replace PHI with reversible tokens

## References

- [HIPAA Security Rule](https://www.hhs.gov/hipaa/for-professionals/security/index.html)
- [45 CFR Part 164 Subpart C](https://www.ecfr.gov/current/title-45/subtitle-A/subchapter-C/part-164/subpart-C)
- [HHS Guidance on De-identification](https://www.hhs.gov/hipaa/for-professionals/privacy/special-topics/de-identification/index.html)
- [HL7 Version 2 Security Considerations](https://www.hl7.org/implement/standards/product_brief.cfm?product_id=185)

## Support

For questions about PHI redaction in HL7DB:

1. Review this documentation
2. Run the test suite: `./bin/test_phi_redaction`
3. Consult the API reference: `include/util/phi_redaction.h`
4. Review example code: `test_phi_redaction.c`

**Remember**: PHI redaction is ONE component of HIPAA compliance. You must also implement:
- Access controls (authentication/authorization)
- Encryption at rest and in transit
- Audit logging
- Incident response procedures
- Business Associate Agreements
- Employee training
- Physical security measures
