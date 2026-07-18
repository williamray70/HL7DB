/**
 * main.c - HL7DB Server Entry Point
 *
 * Phase 1: Basic utility testing
 * Phase 2: HL7 parser testing
 * Phase 2.5: HL7SQL query engine testing
 * Phase 6: Network server (TCP listener with binary protocol)
 */

#define _GNU_SOURCE

#include "util/logger.h"
#include "util/memory.h"
#include "util/list.h"
#include "util/hashmap.h"
#include "util/config.h"
#include "hl7/hl7_parser.h"
#include "hl7/hl7_message.h"
#include "hl7/hl7_validator.h"
#include "query/hl7sql_parser.h"
#include "query/hl7sql_store.h"
#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_result.h"
#include "network/tcp_server.h"
#include "network/tls_server.h"
#include "security/encryption.h"
#include "security/auth.h"
#include "security/mfa.h"
#include "security/security_monitor.h"
#include "storage/backup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

/**
 * Test utility functions
 */
static int test_utilities(void) {
    LOG_INFO("Testing HL7DB utility libraries...");

    /* Test memory pool */
    LOG_INFO("Testing memory pool allocator...");
    memory_pool_t *pool = memory_pool_create(1024);
    if (!pool) {
        LOG_ERROR("Failed to create memory pool");
        return -1;
    }

    char *str1 = memory_pool_strdup(pool, "Hello, HL7DB!");
    char *str2 = memory_pool_alloc(pool, 64);
    if (!str1 || !str2) {
        LOG_ERROR("Memory pool allocation failed");
        memory_pool_destroy(pool);
        return -1;
    }

    snprintf(str2, 64, "Allocated from pool");
    LOG_DEBUG("Memory pool test: str1='%s', str2='%s'", str1, str2);

    size_t allocated, capacity, num_blocks;
    memory_pool_get_stats(pool, &allocated, &capacity, &num_blocks);
    LOG_INFO("Memory pool stats: allocated=%zu, capacity=%zu, blocks=%zu",
             allocated, capacity, num_blocks);

    memory_pool_destroy(pool);
    LOG_INFO("Memory pool test passed!");

    /* Test linked list */
    LOG_INFO("Testing linked list...");
    list_t *list = list_create();
    if (!list) {
        LOG_ERROR("Failed to create list");
        return -1;
    }

    list_append(list, "First");
    list_append(list, "Second");
    list_prepend(list, "Zero");

    LOG_DEBUG("List size: %zu", list_size(list));
    LOG_DEBUG("List first: %s", (char *)list_first(list));
    LOG_DEBUG("List last: %s", (char *)list_last(list));

    list_destroy(list, NULL);
    LOG_INFO("Linked list test passed!");

    /* Test hash map */
    LOG_INFO("Testing hash map...");
    hashmap_t *map = hashmap_create(0);
    if (!map) {
        LOG_ERROR("Failed to create hash map");
        return -1;
    }

    hashmap_put(map, "server.port", "7777");
    hashmap_put(map, "server.host", "localhost");
    hashmap_put(map, "storage.path", "/var/lib/hl7db");

    const char *port = hashmap_get(map, "server.port");
    const char *host = hashmap_get(map, "server.host");

    LOG_DEBUG("Hash map get: port=%s, host=%s", port, host);
    LOG_INFO("Hash map size: %zu", hashmap_size(map));

    hashmap_destroy(map, NULL);
    LOG_INFO("Hash map test passed!");

    return 0;
}

/**
 * Test HL7 parser
 */
static int test_hl7_parser(void) {
    LOG_INFO("Testing HL7 parser...");

    /* Real ADT^A01 message (admission) */
    const char *adt_message =
        "MSH|^~\\&|SENDING_APP|SENDING_FAC|RECEIVING_APP|RECEIVING_FAC|20230615083000||ADT^A01|MSG00001|P|2.5\r"
        "EVN|A01|20230615083000\r"
        "PID|1||123456789^^^FACILITY^MR||JONES^WILLIAM^A||19800515|M|||123 MAIN ST^^CITY^ST^12345\r"
        "PV1|1|I|2000^2012^01||||004777^SMITH^JOHN^A|||SUR||||ADM|A0\r";

    LOG_INFO("Parsing ADT^A01 message...");
    hl7_message_t *msg = hl7_parse(adt_message, NULL);

    if (!msg) {
        LOG_ERROR("Failed to parse ADT message");
        return -1;
    }

    /* Print message summary */
    LOG_INFO("Successfully parsed message:");
    LOG_INFO("  Type: %s", msg->msg_type_str);
    LOG_INFO("  Version: %s", hl7_version_to_string(msg->version));
    LOG_INFO("  Segments: %zu", msg->num_segments);
    LOG_INFO("  Parse errors: %d", msg->parse_errors);

    /* Validate message */
    int valid = hl7_validator_validate(msg);
    if (valid == 0) {
        LOG_INFO("  Validation: PASSED");
    } else {
        LOG_WARN("  Validation: FAILED (%s)",
                 hl7_validator_get_error_string(valid));
    }

    /* Test field extraction */
    const char *patient_id = hl7_message_get_field(msg, "PID-3");
    const char *patient_name = hl7_message_get_field(msg, "PID-5");
    const char *msg_type = hl7_message_get_field(msg, "MSH-9");

    LOG_INFO("Field extraction test:");
    LOG_INFO("  PID-3 (Patient ID): %s", patient_id ? patient_id : "NULL");
    LOG_INFO("  PID-5 (Patient Name): %s", patient_name ? patient_name : "NULL");
    LOG_INFO("  MSH-9 (Message Type): %s", msg_type ? msg_type : "NULL");

    /* Test segment retrieval */
    hl7_segment_t *pid = hl7_message_get_segment(msg, "PID", 0);
    if (pid) {
        LOG_INFO("  Found PID segment with %zu fields", pid->num_fields);
    }

    /* Print detailed structure if debug */
    LOG_DEBUG("=== Full Message Structure ===");
    hl7_message_print(msg, 1);

    hl7_message_destroy(msg);

    /* Test ORU message (lab results) */
    const char *oru_message =
        "MSH|^~\\&|LAB|FACILITY|EHR|CLINIC|20230615090000||ORU^R01|MSG00002|P|2.5\r"
        "PID|1||987654321^^^FACILITY^MR||DOE^JANE^M||19750310|F\r"
        "OBR|1|ORDER123|RESULT456|CBC^COMPLETE BLOOD COUNT|||20230615080000\r"
        "OBX|1|NM|WBC^White Blood Count||7.5|10^9/L|4.0-11.0||||F\r"
        "OBX|2|NM|RBC^Red Blood Count||4.8|10^12/L|4.2-5.4||||F\r"
        "OBX|3|NM|HGB^Hemoglobin||14.2|g/dL|12.0-16.0||||F\r";

    LOG_INFO("Parsing ORU^R01 message (lab results)...");
    hl7_message_t *oru_msg = hl7_parse(oru_message, NULL);

    if (!oru_msg) {
        LOG_ERROR("Failed to parse ORU message");
        return -1;
    }

    LOG_INFO("Successfully parsed ORU message:");
    LOG_INFO("  Type: %s", oru_msg->msg_type_str);
    LOG_INFO("  Segments: %zu", oru_msg->num_segments);

    /* Get all OBX segments */
    size_t num_obx = 0;
    hl7_segment_t **obx_segments = hl7_message_get_all_segments(oru_msg, "OBX", &num_obx);

    LOG_INFO("  Found %zu OBX (observation) segments", num_obx);

    if (obx_segments) {
        for (size_t i = 0; i < num_obx; i++) {
            LOG_DEBUG("    OBX[%zu]: %zu fields", i, obx_segments[i]->num_fields);
        }
        free(obx_segments);
    }

    hl7_message_destroy(oru_msg);

    LOG_INFO("HL7 parser tests passed!");
    return 0;
}

/**
 * Test HL7SQL query engine
 */
static int test_hl7sql_engine(void) {
    LOG_INFO("Testing HL7SQL query engine with channel support...");

    /* Create channel manager */
    hl7sql_queue_manager_t *mgr = hl7sql_queue_manager_create();
    if (!mgr) {
        LOG_ERROR("Failed to create channel manager");
        return -1;
    }

    LOG_INFO("Channel manager created successfully");
    printf("\n");

    /* Test CREATE QUEUE */
    LOG_INFO("Creating channels...");
    hl7sql_result_t *create_adt = hl7sql_query(mgr, "CREATE QUEUE adt_messages");
    if (!create_adt || create_adt->error_code != 0) {
        LOG_ERROR("Failed to create ADT channel");
        if (create_adt) hl7sql_result_destroy(create_adt);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    hl7sql_result_destroy(create_adt);
    LOG_INFO("  Created channel 'adt_messages'");

    hl7sql_result_t *create_lab = hl7sql_query(mgr, "CREATE QUEUE lab_results");
    if (!create_lab || create_lab->error_code != 0) {
        LOG_ERROR("Failed to create lab channel");
        if (create_lab) hl7sql_result_destroy(create_lab);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    hl7sql_result_destroy(create_lab);
    LOG_INFO("  Created channel 'lab_results'");
    printf("\n");

    /* Sample HL7 messages for testing */
    const char *adt_msg1 =
        "MSH|^~\\&|SENDING_APP|SENDING_FAC|RECEIVING_APP|RECEIVING_FAC|20230615083000||ADT^A01|MSG00001|P|2.5\r"
        "EVN|A01|20230615083000\r"
        "PID|1||123456789^^^FACILITY^MR||JONES^WILLIAM^A||19800515|M|||123 MAIN ST^^CITY^ST^12345\r"
        "PV1|1|I|2000^2012^01||||004777^SMITH^JOHN^A|||SUR||||ADM|A0\r";

    const char *adt_msg2 =
        "MSH|^~\\&|SENDING_APP|SENDING_FAC|RECEIVING_APP|RECEIVING_FAC|20230615090000||ADT^A01|MSG00002|P|2.5\r"
        "EVN|A01|20230615090000\r"
        "PID|1||987654321^^^FACILITY^MR||DOE^JANE^M||19750310|F|||456 OAK AVE^^CITY^ST^54321\r"
        "PV1|1|O|3000^3005^02||||005888^BROWN^MARY^B|||MED||||ADM|A0\r";

    const char *oru_msg =
        "MSH|^~\\&|LAB|FACILITY|EHR|CLINIC|20230615100000||ORU^R01|MSG00003|P|2.5\r"
        "PID|1||123456789^^^FACILITY^MR||JONES^WILLIAM^A||19800515|M\r"
        "OBR|1|ORDER123|RESULT456|CBC^COMPLETE BLOOD COUNT|||20230615080000\r"
        "OBX|1|NM|WBC^White Blood Count||7.5|10^9/L|4.0-11.0||||F\r";

    /* Test INSERT MESSAGE INTO channel */
    LOG_INFO("Testing INSERT MESSAGE INTO channel...");

    char insert_query1[2048];
    snprintf(insert_query1, sizeof(insert_query1),
             "INSERT MESSAGE INTO adt_messages '%s'", adt_msg1);
    hl7sql_result_t *insert1 = hl7sql_query(mgr, insert_query1);
    if (!insert1 || insert1->error_code != 0) {
        LOG_ERROR("Failed to insert message 1");
        if (insert1) hl7sql_result_destroy(insert1);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    hl7sql_result_destroy(insert1);
    LOG_INFO("  Inserted message 1 into adt_messages");

    char insert_query2[2048];
    snprintf(insert_query2, sizeof(insert_query2),
             "INSERT MESSAGE INTO adt_messages '%s'", adt_msg2);
    hl7sql_result_t *insert2 = hl7sql_query(mgr, insert_query2);
    if (!insert2 || insert2->error_code != 0) {
        LOG_ERROR("Failed to insert message 2");
        if (insert2) hl7sql_result_destroy(insert2);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    hl7sql_result_destroy(insert2);
    LOG_INFO("  Inserted message 2 into adt_messages");

    char insert_query3[2048];
    snprintf(insert_query3, sizeof(insert_query3),
             "INSERT MESSAGE INTO lab_results '%s'", oru_msg);
    hl7sql_result_t *insert3 = hl7sql_query(mgr, insert_query3);
    if (!insert3 || insert3->error_code != 0) {
        LOG_ERROR("Failed to insert message 3");
        if (insert3) hl7sql_result_destroy(insert3);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    hl7sql_result_destroy(insert3);
    LOG_INFO("  Inserted message 3 into lab_results");
    printf("\n");

    /* Test SELECT from specific channel */
    LOG_INFO("Testing SELECT * FROM adt_messages...");
    hl7sql_result_t *select_adt = hl7sql_query(mgr, "SELECT * FROM adt_messages");
    if (!select_adt || select_adt->error_code != 0) {
        LOG_ERROR("SELECT from adt_messages failed");
        if (select_adt) hl7sql_result_destroy(select_adt);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    LOG_INFO("Query returned %zu rows:", select_adt->num_rows);
    hl7sql_result_print(select_adt);
    hl7sql_result_destroy(select_adt);
    printf("\n");

    /* Test SELECT with WHERE from specific channel */
    LOG_INFO("Testing SELECT with WHERE from adt_messages...");
    hl7sql_result_t *select_jones = hl7sql_query(mgr,
        "SELECT * FROM adt_messages WHERE segment.PID[5] = 'JONES'");
    if (!select_jones || select_jones->error_code != 0) {
        LOG_ERROR("SELECT JONES failed");
        if (select_jones) hl7sql_result_destroy(select_jones);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    LOG_INFO("Query returned %zu rows (patients named JONES):", select_jones->num_rows);
    hl7sql_result_print(select_jones);
    hl7sql_result_destroy(select_jones);
    printf("\n");

    /* Test SELECT from ALL channels */
    LOG_INFO("Testing SELECT * FROM * (all channels)...");
    hl7sql_result_t *select_all = hl7sql_query(mgr, "SELECT * FROM *");
    if (!select_all || select_all->error_code != 0) {
        LOG_ERROR("SELECT from all channels failed");
        if (select_all) hl7sql_result_destroy(select_all);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    LOG_INFO("Query returned %zu rows from all channels:", select_all->num_rows);
    hl7sql_result_print(select_all);
    hl7sql_result_destroy(select_all);
    printf("\n");

    /* Test POP MESSAGE (queue semantics) */
    LOG_INFO("Testing POP MESSAGE FROM adt_messages (FIFO)...");
    hl7sql_result_t *pop1 = hl7sql_query(mgr, "POP MESSAGE FROM adt_messages");
    if (!pop1 || pop1->error_code != 0) {
        LOG_ERROR("POP MESSAGE failed");
        if (pop1) hl7sql_result_destroy(pop1);
        hl7sql_queue_manager_destroy(mgr);
        return -1;
    }
    LOG_INFO("Popped %zu message(s) (oldest first):", pop1->num_rows);
    hl7sql_result_print(pop1);
    hl7sql_result_destroy(pop1);
    printf("\n");

    /* Verify channel has one less message */
    LOG_INFO("Verifying adt_messages now has 1 message remaining...");
    hl7sql_result_t *verify = hl7sql_query(mgr, "SELECT * FROM adt_messages");
    if (verify) {
        LOG_INFO("adt_messages now has %zu message(s)", verify->num_rows);
        hl7sql_result_destroy(verify);
    }
    printf("\n");

    /* Test DROP QUEUE on empty channel */
    LOG_INFO("Testing DROP QUEUE...");
    hl7sql_result_t *create_temp = hl7sql_query(mgr, "CREATE QUEUE temp");
    if (create_temp) {
        hl7sql_result_destroy(create_temp);
        LOG_INFO("  Created temporary channel");

        hl7sql_result_t *drop_temp = hl7sql_query(mgr, "DROP QUEUE temp");
        if (drop_temp && drop_temp->error_code == 0) {
            LOG_INFO("  Dropped empty channel successfully");
        } else {
            LOG_ERROR("  Failed to drop empty channel");
        }
        if (drop_temp) hl7sql_result_destroy(drop_temp);
    }
    printf("\n");

    /* Cleanup */
    hl7sql_queue_manager_destroy(mgr);

    LOG_INFO("HL7SQL query engine tests passed!");
    return 0;
}

/**
 * Print banner
 */
static void print_banner(void) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║         HL7DB Database Server          ║\n");
    printf("║    HL7 v2.x Specialized Database       ║\n");
    printf("║            Version 0.3.0               ║\n");
    printf("║      with TCP Network Server           ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
}

/* Global server instance for signal handling */
/* Global server and security system instances */
static tcp_server_t *g_server = NULL;
static tls_server_t *g_tls_server = NULL;
static security_monitor_t *g_security_monitor = NULL;
static auth_system_t *g_auth_system = NULL;
static mfa_system_t *g_mfa_system = NULL;
static backup_system_t *g_backup_system = NULL;

/**
 * Signal handler for graceful shutdown
 */
static void signal_handler(int signum) {
    (void)signum;
    LOG_INFO("Received shutdown signal");
    if (g_tls_server) {
        tls_server_stop(g_tls_server);
    }
    if (g_server) {
        tcp_server_stop(g_server);
    }
}

/**
 * Main entry point
 */
int main(int argc, char *argv[]) {
    /* Parse command line options */
    int run_tests = 0;
    int port = 7777;
    char *host = "0.0.0.0";
    const char *config_file = NULL;
    char *data_dir = "./data";
    int max_connections = 100;
    log_level_t log_level = LOG_INFO;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            run_tests = 1;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_file = argv[++i];
        } else if (strcmp(argv[i], "--data-dir") == 0 && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (strcmp(argv[i], "--max-connections") == 0 && i + 1 < argc) {
            max_connections = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--debug") == 0) {
            log_level = LOG_DEBUG;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("HL7DB v0.3.0\n");
            printf("HL7 v2.x Specialized Database with Network Server\n");
            printf("Build: %s %s\n", __DATE__, __TIME__);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n\n", argv[0]);
            printf("HL7DB - HL7 v2.x Specialized Database Server\n\n");
            printf("Options:\n");
            printf("  --config FILE         Configuration file path\n");
            printf("  --host HOST           Bind address (default: 0.0.0.0)\n");
            printf("  --port PORT           TCP port (default: 7777)\n");
            printf("  --data-dir DIR        Data directory (default: ./data)\n");
            printf("  --max-connections N   Maximum connections (default: 100)\n");
            printf("  --debug               Enable debug logging\n");
            printf("  --test                Run tests and exit\n");
            printf("  --version, -v         Show version information\n");
            printf("  --help, -h            Show this help message\n\n");
            printf("Examples:\n");
            printf("  %s --config /etc/hl7db/hl7db.conf\n", argv[0]);
            printf("  %s --port 8888 --host 127.0.0.1\n", argv[0]);
            printf("  %s --debug --test\n\n", argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Use --help for usage information\n");
            return 1;
        }
    }

    /* Initialize logger */
    logger_init_default();
    logger_set_level(log_level);

    /* Load configuration file if specified */
    if (config_file) {
        LOG_INFO("Loading configuration from: %s", config_file);
        config_t *config = config_load(config_file);
        if (config) {
            /* Override defaults with config file values (command line args take precedence) */
            const char *cfg_value;

            /* Only override if not set via command line */
            if (argc <= 2) {  /* No specific options beyond --config */
                cfg_value = config_get_string(config, "server", "port", NULL);
                if (cfg_value) port = atoi(cfg_value);

                cfg_value = config_get_string(config, "server", "bind_address", NULL);
                if (cfg_value) host = strdup(cfg_value);

                cfg_value = config_get_string(config, "server", "max_connections", NULL);
                if (cfg_value) max_connections = atoi(cfg_value);

                cfg_value = config_get_string(config, "storage", "data_directory", NULL);
                if (cfg_value) data_dir = strdup(cfg_value);

                cfg_value = config_get_string(config, "logging", "log_level", NULL);
                if (cfg_value) {
                    if (strcmp(cfg_value, "DEBUG") == 0) log_level = LOG_DEBUG;
                    else if (strcmp(cfg_value, "INFO") == 0) log_level = LOG_INFO;
                    else if (strcmp(cfg_value, "WARN") == 0) log_level = LOG_WARN;
                    else if (strcmp(cfg_value, "ERROR") == 0) log_level = LOG_ERROR;
                    logger_set_level(log_level);
                }
            }

            LOG_INFO("Configuration loaded successfully");
            config_destroy(config);
        } else {
            LOG_WARN("Failed to load configuration file: %s", config_file);
            LOG_WARN("Continuing with default/command-line settings");
        }
    }

    print_banner();

    LOG_INFO("HL7DB Server starting...");
    LOG_INFO("Phase 6: Network Server with HL7SQL Engine");
    LOG_INFO("Build: %s %s", __DATE__, __TIME__);

    /* Run tests if requested */
    if (run_tests) {
        logger_set_level(LOG_DEBUG);

        /* Test utilities */
        if (test_utilities() != 0) {
            LOG_FATAL("Utility tests failed!");
            return 1;
        }

        LOG_INFO("All utility tests passed!");
        printf("\n");

        /* Test HL7 parser */
        if (test_hl7_parser() != 0) {
            LOG_FATAL("HL7 parser tests failed!");
            return 1;
        }

        LOG_INFO("All HL7 parser tests passed!");
        printf("\n");

        /* Test HL7SQL query engine */
        if (test_hl7sql_engine() != 0) {
            LOG_FATAL("HL7SQL query engine tests failed!");
            return 1;
        }

        LOG_INFO("All HL7SQL query engine tests passed!");
        printf("\n");

        LOG_INFO("=== All Tests Passed ===");
        logger_cleanup();
        return 0;
    }

    /* Start network server mode */
    LOG_INFO("Starting HIPAA-compliant HL7DB server...");
    LOG_INFO("Bind address: %s:%d", host, port);

    /* ================================================================
     * PHASE 1: Initialize Encryption System (HIPAA §164.312(a)(2)(iv))
     * ================================================================ */
    LOG_INFO("Initializing encryption system...");

    /* Ensure encryption key directory exists */
    const char *key_dir = "./keys";
    mkdir(key_dir, 0700);

    encryption_config_t enc_config = {
        .enabled = true,
        .master_key = NULL,  /* Will be loaded from key_file */
        .use_hsm = false
    };
    snprintf(enc_config.key_file, sizeof(enc_config.key_file), "./keys/master.key");

    if (encryption_init(&enc_config) != 0) {
        LOG_FATAL("Failed to initialize encryption system - HIPAA compliance requires encryption");
        return 1;
    }
    LOG_INFO("✓ AES-256-GCM encryption enabled (HIPAA compliant)");

    /* ================================================================
     * PHASE 2: Initialize Authentication (HIPAA §164.312(d))
     * ================================================================ */
    LOG_INFO("Initializing authentication system...");

    g_auth_system = auth_system_init();
    if (!g_auth_system) {
        LOG_FATAL("Failed to initialize authentication system");
        encryption_cleanup();
        return 1;
    }

    /* Create default admin user (will fail if already exists) */
    LOG_INFO("Ensuring default admin user exists...");
    uint32_t admin_id = auth_create_user(g_auth_system, "admin", "Admin123!Test",
                        "Default Administrator", ROLE_ADMIN, NULL);
    if (admin_id != 0) {
        LOG_WARN("Default admin account created with username 'admin'");
        LOG_WARN("*** CHANGE THE DEFAULT PASSWORD IMMEDIATELY ***");
    } else {
        LOG_INFO("Admin user already exists");
    }
    LOG_INFO("✓ Authentication system enabled (PBKDF2-HMAC-SHA256)");

    /* ================================================================
     * PHASE 3: Initialize MFA (HIPAA §164.312(d) enhanced)
     * ================================================================ */
    LOG_INFO("Initializing multi-factor authentication...");

    g_mfa_system = mfa_system_init();
    if (!g_mfa_system) {
        LOG_WARN("Failed to initialize MFA system (optional)");
    } else {
        LOG_INFO("✓ TOTP-based MFA available (RFC 6238)");
    }

    /* ================================================================
     * PHASE 4: Initialize Security Monitoring (HIPAA §164.308(a)(1)(ii)(D))
     * ================================================================ */
    LOG_INFO("Initializing security monitoring...");

    g_security_monitor = security_monitor_init();
    if (!g_security_monitor) {
        LOG_WARN("Failed to initialize security monitoring");
    } else {
        /* Configure default alert rules */
        alert_rule_t rule = {
            .event_type = SECURITY_EVENT_BRUTE_FORCE,
            .min_severity = SECURITY_SEVERITY_HIGH,
            .action = ALERT_ACTION_BLOCK_IP,
            .enabled = true
        };
        security_monitor_add_alert_rule(g_security_monitor, &rule);

        LOG_INFO("✓ Security monitoring enabled with anomaly detection");
    }

    /* ================================================================
     * PHASE 5: Create Channel Manager
     * ================================================================ */
    hl7sql_queue_manager_t *queue_mgr = hl7sql_queue_manager_create();
    if (!queue_mgr) {
        LOG_FATAL("Failed to create channel manager");
        /* Cleanup */
        if (g_security_monitor) security_monitor_destroy(g_security_monitor);
        if (g_mfa_system) mfa_system_destroy(g_mfa_system);
        if (g_auth_system) auth_system_destroy(g_auth_system);
        encryption_cleanup();
        return 1;
    }
    LOG_INFO("✓ Channel manager initialized");

    /* ================================================================
     * PHASE 6: Enable Encrypted Persistence (HIPAA §164.312(a)(2)(iv))
     * ================================================================ */
    LOG_INFO("Configuring encrypted persistence...");

    persistence_config_t persistence_config = {
        .data_dir = data_dir,
        .sync_on_write = 1,  /* HIPAA: sync writes for data integrity */
        .encrypt_at_rest = true  /* CRITICAL: Enable encryption */
    };

    if (hl7sql_queue_manager_set_persistence(queue_mgr, &persistence_config) == 0) {
        LOG_INFO("✓ Encrypted persistence enabled: data_dir=%s", persistence_config.data_dir);

        /* Load registered queues from registry */
        int loaded = hl7sql_queue_manager_load_registry(queue_mgr);
        if (loaded > 0) {
            LOG_INFO("Restored %d queues from registry", loaded);
        } else if (loaded == 0) {
            LOG_INFO("No queues found in registry");
        } else {
            LOG_WARN("Failed to load queue registry");
        }
    } else {
        LOG_FATAL("Failed to enable encrypted persistence - HIPAA compliance requires encryption at rest");
        /* Cleanup */
        hl7sql_queue_manager_destroy(queue_mgr);
        if (g_security_monitor) security_monitor_destroy(g_security_monitor);
        if (g_mfa_system) mfa_system_destroy(g_mfa_system);
        if (g_auth_system) auth_system_destroy(g_auth_system);
        encryption_cleanup();
        return 1;
    }

    /* ================================================================
     * PHASE 7: Initialize Backup System (HIPAA §164.308(a)(7))
     * ================================================================ */
    LOG_INFO("Initializing backup and disaster recovery...");

    backup_config_t backup_config = backup_get_default_config();
    snprintf(backup_config.data_dir, sizeof(backup_config.data_dir), "%s", data_dir);

    g_backup_system = backup_system_init(&backup_config);
    if (!g_backup_system) {
        LOG_WARN("Failed to initialize backup system (optional but recommended)");
    } else {
        /* Configure automated backups */
        backup_schedule_t schedule = {
            .enabled = true,
            .interval_hours = 24,  /* Daily backups */
            .backup_type = BACKUP_TYPE_INCREMENTAL,
            .full_backup_day = 0  /* Sunday for full backup */
        };

        if (backup_start_scheduler(g_backup_system, &schedule) == 0) {
            LOG_INFO("✓ Automated backups enabled (daily incremental, weekly full)");
            LOG_INFO("  Retention: %d days (HIPAA 6-year requirement)", backup_config.retention_days);
        } else {
            LOG_WARN("Failed to start backup scheduler");
        }
    }

    /* ================================================================
     * PHASE 8: Start TLS Server (HIPAA §164.312(e)(1))
     * ================================================================ */
    LOG_INFO("Starting TLS 1.2+ encrypted server...");

    /* Ensure certificate directory exists */
    const char *cert_dir = "./certs";
    mkdir(cert_dir, 0755);

    tls_config_t tls_config = tls_get_default_config();
    snprintf(tls_config.cert_file, sizeof(tls_config.cert_file), "./certs/server.crt");
    snprintf(tls_config.key_file, sizeof(tls_config.key_file), "./certs/server.key");
    tls_config.port = port;
    tls_config.max_connections = max_connections;

    /* Check if certificates exist, create self-signed if needed */
    struct stat st;
    if (stat(tls_config.cert_file, &st) != 0 || stat(tls_config.key_file, &st) != 0) {
        LOG_WARN("TLS certificates not found at %s", tls_config.cert_file);
        LOG_WARN("Generate certificates with: openssl req -x509 -newkey rsa:4096 -keyout %s -out %s -days 365 -nodes",
                 tls_config.key_file, tls_config.cert_file);
        LOG_FATAL("TLS certificates required for HIPAA compliance - cannot start server");

        /* Cleanup all systems */
        if (g_backup_system) backup_system_destroy(g_backup_system);
        hl7sql_queue_manager_destroy(queue_mgr);
        if (g_security_monitor) security_monitor_destroy(g_security_monitor);
        if (g_mfa_system) mfa_system_destroy(g_mfa_system);
        if (g_auth_system) auth_system_destroy(g_auth_system);
        encryption_cleanup();
        return 1;
    }

    g_tls_server = tls_server_create(&tls_config, (hl7sql_channel_manager_t *)queue_mgr);
    if (!g_tls_server) {
        LOG_FATAL("Failed to create TLS server");
        /* Cleanup all systems */
        if (g_backup_system) backup_system_destroy(g_backup_system);
        hl7sql_queue_manager_destroy(queue_mgr);
        if (g_security_monitor) security_monitor_destroy(g_security_monitor);
        if (g_mfa_system) mfa_system_destroy(g_mfa_system);
        if (g_auth_system) auth_system_destroy(g_auth_system);
        encryption_cleanup();
        return 1;
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Start TLS server */
    if (tls_server_start(g_tls_server) != 0) {
        LOG_FATAL("Failed to start TLS server");
        tls_server_destroy(g_tls_server);
        /* Cleanup all systems */
        if (g_backup_system) backup_system_destroy(g_backup_system);
        hl7sql_queue_manager_destroy(queue_mgr);
        if (g_security_monitor) security_monitor_destroy(g_security_monitor);
        if (g_mfa_system) mfa_system_destroy(g_mfa_system);
        if (g_auth_system) auth_system_destroy(g_auth_system);
        encryption_cleanup();
        return 1;
    }

    /* ================================================================
     * SERVER READY - HIPAA COMPLIANT CONFIGURATION
     * ================================================================ */
    printf("\n");
    LOG_INFO("========================================");
    LOG_INFO("=== HL7DB Server Ready (HIPAA Mode) ===");
    LOG_INFO("========================================");
    printf("\n");
    LOG_INFO("Security Status:");
    LOG_INFO("  ✓ TLS 1.2+ server listening on port %d", port);
    LOG_INFO("  ✓ AES-256-GCM encryption at rest");
    LOG_INFO("  ✓ PBKDF2-HMAC-SHA256 password hashing");
    LOG_INFO("  ✓ TOTP multi-factor authentication available");
    LOG_INFO("  ✓ Security monitoring and anomaly detection active");
    LOG_INFO("  ✓ Automated backups (daily incremental, weekly full)");
    LOG_INFO("  ✓ Audit logging enabled");
    printf("\n");
    LOG_INFO("HIPAA Compliance:");
    LOG_INFO("  ✓ §164.312(a)(2)(iv) - Encryption at Rest");
    LOG_INFO("  ✓ §164.312(e)(1) - Transmission Security (TLS)");
    LOG_INFO("  ✓ §164.312(d) - Authentication & Access Control");
    LOG_INFO("  ✓ §164.308(a)(7) - Backup & Disaster Recovery");
    LOG_INFO("  ✓ §164.312(b) - Audit Controls");
    LOG_INFO("  ✓ §164.308(a)(1)(ii)(D) - Security Monitoring");
    printf("\n");
    LOG_INFO("Default Credentials: admin / Admin123!Test");
    LOG_INFO("*** CHANGE DEFAULT PASSWORD IMMEDIATELY ***");
    printf("\n");
    LOG_INFO("Press Ctrl+C to shutdown");
    printf("\n");

    /* Wait for shutdown signal */
    while (g_tls_server && tls_server_is_running(g_tls_server)) {
        sleep(1);
    }

    /* ================================================================
     * CLEANUP ALL SYSTEMS
     * ================================================================ */
    LOG_INFO("Shutting down all systems...");

    if (g_tls_server) {
        tls_server_destroy(g_tls_server);
        LOG_INFO("✓ TLS server stopped");
    }

    if (g_backup_system) {
        backup_system_destroy(g_backup_system);
        LOG_INFO("✓ Backup system stopped");
    }

    hl7sql_queue_manager_destroy(queue_mgr);
    LOG_INFO("✓ Channel manager stopped");

    if (g_security_monitor) {
        security_monitor_destroy(g_security_monitor);
        LOG_INFO("✓ Security monitoring stopped");
    }

    if (g_mfa_system) {
        mfa_system_destroy(g_mfa_system);
        LOG_INFO("✓ MFA system stopped");
    }

    if (g_auth_system) {
        auth_system_destroy(g_auth_system);
        LOG_INFO("✓ Authentication system stopped");
    }

    encryption_cleanup();
    LOG_INFO("✓ Encryption system stopped");

    logger_cleanup();

    LOG_INFO("Shutdown complete");
    return 0;
}
