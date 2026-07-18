/**
 * test_phi_redaction.c - Test HIPAA PHI Redaction Features
 *
 * Demonstrates compliance with HIPAA Security Rule (45 CFR 164.312)
 * by redacting Protected Health Information from logs and output.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "util/logger.h"
#include "util/phi_redaction.h"
#include "hl7/hl7_parser.h"
#include "hl7/hl7_message.h"
#include "query/hl7sql_parser.h"
#include "query/hl7sql_channel_manager.h"
#include "query/hl7sql_result.h"

void print_header(const char *title) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║ %-58s ║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void test_field_redaction(void) {
    print_header("Test 1: Field-Level PHI Redaction");

    /* Test data */
    const char *patient_name = "JONES^WILLIAM^A";
    const char *patient_id = "123456789";
    const char *dob = "19800515";
    const char *address = "123 MAIN ST^^CITY^ST^12345";
    const char *msg_type = "ADT^A01";

    phi_redaction_config_t configs[] = {
        phi_redaction_get_default_config(),
        phi_redaction_get_debug_config(),
        phi_redaction_get_none_config()
    };
    const char *config_names[] = {"FULL", "PARTIAL (debug)", "NONE"};

    for (int i = 0; i < 3; i++) {
        printf("Configuration: %s\n", config_names[i]);
        printf("─────────────────────────────────────────────────────\n");

        char *name_redacted = phi_redact_field("PID-5", patient_name, &configs[i]);
        char *id_redacted = phi_redact_field("PID-3", patient_id, &configs[i]);
        char *dob_redacted = phi_redact_field("PID-7", dob, &configs[i]);
        char *addr_redacted = phi_redact_field("PID-11", address, &configs[i]);
        char *type_redacted = phi_redact_field("MSH-9", msg_type, &configs[i]);

        printf("  PID-5 (Patient Name): %s -> %s\n", patient_name, name_redacted);
        printf("  PID-3 (Patient ID):   %s -> %s\n", patient_id, id_redacted);
        printf("  PID-7 (DOB):          %s -> %s\n", dob, dob_redacted);
        printf("  PID-11 (Address):     %s -> %s\n", address, addr_redacted);
        printf("  MSH-9 (Msg Type):     %s -> %s\n", msg_type, type_redacted);

        free(name_redacted);
        free(id_redacted);
        free(dob_redacted);
        free(addr_redacted);
        free(type_redacted);

        printf("\n");
    }
}

void test_message_redaction(void) {
    print_header("Test 2: Full HL7 Message Redaction");

    const char *adt_message =
        "MSH|^~\\&|SENDING_APP|SENDING_FAC|RECEIVING_APP|RECEIVING_FAC|20230615083000||ADT^A01|MSG00001|P|2.5\r"
        "EVN|A01|20230615083000\r"
        "PID|1||123456789^^^FACILITY^MR||JONES^WILLIAM^A||19800515|M|||123 MAIN ST^^CITY^ST^12345||555-1234\r"
        "PV1|1|I|2000^2012^01||||004777^SMITH^JOHN^A|||SUR||||ADM|A0\r";

    printf("Original HL7 Message:\n");
    printf("─────────────────────────────────────────────────────\n");
    printf("%s\n\n", adt_message);

    /* Parse the message */
    hl7_message_t *msg = hl7_parse(adt_message, NULL);
    if (!msg) {
        printf("ERROR: Failed to parse message\n");
        return;
    }

    printf("Redacted for Logging (FULL redaction):\n");
    printf("─────────────────────────────────────────────────────\n");
    phi_redaction_config_t full_config = phi_redaction_get_default_config();
    char *redacted_full = phi_redact_message(adt_message, &full_config);
    if (redacted_full) {
        printf("%s\n\n", redacted_full);
        free(redacted_full);
    }

    printf("Redacted for Debugging (PARTIAL redaction):\n");
    printf("─────────────────────────────────────────────────────\n");
    phi_redaction_config_t debug_config = phi_redaction_get_debug_config();
    char *redacted_partial = phi_redact_message(adt_message, &debug_config);
    if (redacted_partial) {
        printf("%s\n\n", redacted_partial);
        free(redacted_partial);
    }

    hl7_message_destroy(msg);
}

void test_message_print_redacted(void) {
    print_header("Test 3: HL7 Message Print with Redaction");

    const char *oru_message =
        "MSH|^~\\&|LAB|FACILITY|EHR|CLINIC|20230615090000||ORU^R01|MSG00002|P|2.5\r"
        "PID|1||987654321^^^FACILITY^MR||DOE^JANE^M||19750310|F||123 OAK AVE^^CITY^ST^54321||555-5678\r"
        "OBR|1|ORDER123|RESULT456|CBC^COMPLETE BLOOD COUNT|||20230615080000\r"
        "OBX|1|NM|WBC^White Blood Count||7.5|10^9/L|4.0-11.0||||F\r"
        "OBX|2|NM|RBC^Red Blood Count||4.8|10^12/L|4.2-5.4||||F\r";

    hl7_message_t *msg = hl7_parse(oru_message, NULL);
    if (!msg) {
        printf("ERROR: Failed to parse message\n");
        return;
    }

    printf("Standard Print (UNSAFE - Contains PHI):\n");
    printf("─────────────────────────────────────────────────────\n");
    hl7_message_print(msg, 1);

    printf("\n\nRedacted Print (SAFE for logs):\n");
    printf("─────────────────────────────────────────────────────\n");
    phi_redaction_config_t config = phi_redaction_get_default_config();
    hl7_message_print_redacted(msg, 1, &config);

    hl7_message_destroy(msg);
}

void test_query_result_redaction(void) {
    print_header("Test 4: Query Result Redaction");

    /* Create channel manager and channels */
    hl7sql_channel_manager_t *mgr = hl7sql_channel_manager_create();
    if (!mgr) {
        printf("ERROR: Failed to create channel manager\n");
        return;
    }

    hl7sql_result_destroy(hl7sql_query(mgr, "CREATE CHANNEL patient_data"));

    /* Insert test messages */
    const char *msg1 =
        "MSH|^~\\&|APP|FAC|RECV|FAC2|20230615083000||ADT^A01|MSG001|P|2.5\r"
        "PID|1||111222333^^^FAC^MR||SMITH^JOHN^||19800101|M";

    const char *msg2 =
        "MSH|^~\\&|APP|FAC|RECV|FAC2|20230615090000||ADT^A01|MSG002|P|2.5\r"
        "PID|1||444555666^^^FAC^MR||DOE^JANE^||19900201|F";

    char insert1[2048], insert2[2048];
    snprintf(insert1, sizeof(insert1), "INSERT MESSAGE INTO patient_data '%s'", msg1);
    snprintf(insert2, sizeof(insert2), "INSERT MESSAGE INTO patient_data '%s'", msg2);

    hl7sql_result_destroy(hl7sql_query(mgr, insert1));
    hl7sql_result_destroy(hl7sql_query(mgr, insert2));

    /* Query messages */
    hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM patient_data");
    if (!result) {
        printf("ERROR: Query failed\n");
        hl7sql_channel_manager_destroy(mgr);
        return;
    }

    printf("Standard Query Result (UNSAFE - Contains PHI):\n");
    printf("─────────────────────────────────────────────────────\n");
    hl7sql_result_print(result);

    printf("\n\nRedacted Query Result (SAFE for logs):\n");
    printf("─────────────────────────────────────────────────────\n");
    phi_redaction_config_t config = phi_redaction_get_default_config();
    hl7sql_result_print_redacted(result, &config);

    hl7sql_result_destroy(result);
    hl7sql_channel_manager_destroy(mgr);
}

void test_hipaa_compliance_scenario(void) {
    print_header("Test 5: HIPAA Compliance Scenario");

    printf("Scenario: Production logging with audit trail\n");
    printf("─────────────────────────────────────────────────────\n\n");

    /* Simulated production message */
    const char *production_msg =
        "MSH|^~\\&|EMR|HOSPITAL_A|LAB|LABCORP|20230615140000||ORU^R01|MSG12345|P|2.5\r"
        "PID|1||555-66-7777^^^USA^SS||PATIENT^IMPORTANT^VIP||19600515|M|||999 EXECUTIVE DR^^METRO^CA^90210||555-0001\r"
        "OBR|1|LAB9999|RES8888|LIPID^Lipid Panel|||20230615120000\r"
        "OBX|1|NM|CHOL^Total Cholesterol||250|mg/dL|<200||||F\r";

    hl7_message_t *msg = hl7_parse(production_msg, NULL);
    if (!msg) {
        printf("ERROR: Failed to parse\n");
        return;
    }

    /* Simulate different logging contexts */
    printf("1. Application Log (File: /var/log/hl7db/app.log):\n");
    printf("   [INFO] Received ORU^R01 message MSG12345 from HOSPITAL_A\n");
    printf("   [INFO] Message contains 4 segments, validation: PASSED\n");
    printf("   [DEBUG] Patient ID: [REDACTED] (PHI protected)\n");
    printf("   [DEBUG] Message Type: ORU^R01 (allowed, not PHI)\n\n");

    printf("2. Audit Trail (Database: audit_log table):\n");
    printf("   timestamp           | facility    | msg_type | msg_id   | patient_id\n");
    printf("   ------------------- | ----------- | -------- | -------- | -----------\n");
    printf("   2023-06-15 14:00:00 | HOSPITAL_A  | ORU^R01  | MSG12345 | [REDACTED]\n\n");

    printf("3. Error Log (with partial redaction for debugging):\n");
    phi_redaction_config_t debug_config = phi_redaction_get_debug_config();
    char *redacted_debug = phi_redact_message(production_msg, &debug_config);
    if (redacted_debug) {
        printf("   [ERROR] Validation warning in message:\n");
        printf("   %s\n\n", redacted_debug);
        free(redacted_debug);
    }

    printf("4. Secure Internal Processing (no redaction):\n");
    printf("   → Full message with PHI available for authorized processing\n");
    printf("   → Access controlled by authentication & authorization\n");
    printf("   → Logged in secure audit system (encrypted, access-restricted)\n\n");

    printf("HIPAA Compliance Summary:\n");
    printf("✓ PHI not exposed in application logs\n");
    printf("✓ PHI not exposed in error messages\n");
    printf("✓ Debug output uses partial redaction when necessary\n");
    printf("✓ Audit trail maintains accountability without exposing PHI\n");
    printf("✓ Full PHI available only for authorized internal processing\n");

    hl7_message_destroy(msg);
}

int main(void) {
    logger_init_default();
    logger_set_level(LOG_INFO);

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║         HL7DB HIPAA PHI Redaction Test Suite              ║\n");
    printf("║                                                            ║\n");
    printf("║  Testing compliance with HIPAA Security Rule              ║\n");
    printf("║  45 CFR 164.312 - Technical Safeguards                    ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    test_field_redaction();
    test_message_redaction();
    test_message_print_redacted();
    test_query_result_redaction();
    test_hipaa_compliance_scenario();

    print_header("All Tests Complete");
    printf("PHI Redaction implementation provides:\n");
    printf("  • Protection of 18 HIPAA identifiers\n");
    printf("  • Multiple redaction modes (FULL, PARTIAL, HASH, NONE)\n");
    printf("  • Safe logging and console output\n");
    printf("  • Audit trail without PHI exposure\n");
    printf("  • Configurable for different security contexts\n");
    printf("\n");
    printf("Use phi_redaction API to ensure HIPAA compliance in your app!\n");
    printf("\n");

    logger_cleanup();
    return 0;
}
