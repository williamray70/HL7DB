/**
 * test_registry_integration.c - Integration test for channel registry
 *
 * Tests the complete lifecycle: create channels, insert messages,
 * restart server, verify channels and messages are restored.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_executor.h"
#include "query/hl7sql_ast.h"
#include "storage/persistence.h"
#include "util/logger.h"
#include "hl7/hl7_parser.h"

#define TEST_DATA_DIR "./test_data_integration"

static int test_count = 0;
static int test_passed = 0;

static void test_assert(int condition, const char *message) {
    test_count++;
    if (condition) {
        printf("✓ Test %d passed: %s\n", test_count, message);
        test_passed++;
    } else {
        printf("✗ Test %d FAILED: %s\n", test_count, message);
    }
}

static void cleanup_test_data(void) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", TEST_DATA_DIR);
    int ret = system(cmd);
    (void)ret;
}

int main(void) {
    printf("=== Channel Registry Integration Test ===\n\n");

    /* Initialize logger */
    logger_config_t log_config = {
        .min_level = LOG_INFO,
        .file = NULL,
        .use_color = 1,
        .include_timestamp = 1,
        .include_level = 1,
        .include_location = 0
    };
    logger_init(&log_config);

    cleanup_test_data();

    /* === PHASE 1: Initial Server Setup === */
    printf("--- Phase 1: Creating channels and inserting messages ---\n");

    persistence_config_t *pconfig = calloc(1, sizeof(persistence_config_t));
    pconfig->data_dir = strdup(TEST_DATA_DIR);
    pconfig->sync_on_write = 1;

    persistence_init(pconfig);

    hl7sql_queue_manager_t *mgr = hl7sql_queue_manager_create();
    hl7sql_queue_manager_set_persistence(mgr, pconfig);

    /* Create channels */
    int ret = hl7sql_queue_manager_create_queue(mgr, "admissions");
    test_assert(ret == 0, "Created 'admissions' queue");

    ret = hl7sql_queue_manager_create_queue(mgr, "lab_results");
    test_assert(ret == 0, "Created 'lab_results' queue");

    ret = hl7sql_queue_manager_create_queue(mgr, "pharmacy");
    test_assert(ret == 0, "Created 'pharmacy' queue");

    /* Insert messages into channels */
    const char *msg1 = "MSH|^~\\&|ADT|HOSPITAL|SYS|CENTRAL|20240101120000||ADT^A01|MSG001|P|2.5\rPID|1||12345||DOE^JOHN";
    const char *msg2 = "MSH|^~\\&|LAB|HOSPITAL|SYS|CENTRAL|20240101120001||ORU^R01|MSG002|P|2.5\rOBR|1||LAB123";
    const char *msg3 = "MSH|^~\\&|PHARM|HOSPITAL|SYS|CENTRAL|20240101120002||RDE^O11|MSG003|P|2.5\rORC|NW";

    hl7_message_t *hl7_msg1 = hl7_parse(msg1, NULL);
    hl7_message_t *hl7_msg2 = hl7_parse(msg2, NULL);
    hl7_message_t *hl7_msg3 = hl7_parse(msg3, NULL);

    hl7sql_store_t *adm_store = hl7sql_queue_manager_get_queue(mgr, "admissions");
    hl7sql_store_t *lab_store = hl7sql_queue_manager_get_queue(mgr, "lab_results");
    hl7sql_store_t *pharm_store = hl7sql_queue_manager_get_queue(mgr, "pharmacy");

    hl7sql_store_insert(adm_store, hl7_msg1);
    hl7sql_store_insert(lab_store, hl7_msg2);
    hl7sql_store_insert(pharm_store, hl7_msg3);

    test_assert(hl7sql_store_size(adm_store) == 1, "Admissions has 1 message");
    test_assert(hl7sql_store_size(lab_store) == 1, "Lab results has 1 message");
    test_assert(hl7sql_store_size(pharm_store) == 1, "Pharmacy has 1 message");

    /* Shutdown (destroy manager) */
    hl7sql_queue_manager_destroy(mgr);
    persistence_cleanup();

    /* === PHASE 2: Server Restart === */
    printf("\n--- Phase 2: Simulating server restart ---\n");

    persistence_init(pconfig);

    mgr = hl7sql_queue_manager_create();
    hl7sql_queue_manager_set_persistence(mgr, pconfig);

    /* Load registry - this should restore all channels */
    int loaded = hl7sql_queue_manager_load_registry(mgr);
    test_assert(loaded == 3, "Loaded 3 queues from registry");

    /* Verify all channels exist */
    test_assert(hl7sql_queue_manager_has_queue(mgr, "admissions"),
                "Admissions queue restored");
    test_assert(hl7sql_queue_manager_has_queue(mgr, "lab_results"),
                "Lab results queue restored");
    test_assert(hl7sql_queue_manager_has_queue(mgr, "pharmacy"),
                "Pharmacy queue restored");

    /* Verify messages were restored */
    adm_store = hl7sql_queue_manager_get_queue(mgr, "admissions");
    lab_store = hl7sql_queue_manager_get_queue(mgr, "lab_results");
    pharm_store = hl7sql_queue_manager_get_queue(mgr, "pharmacy");

    test_assert(hl7sql_store_size(adm_store) == 1,
                "Admissions has 1 message after restart");
    test_assert(hl7sql_store_size(lab_store) == 1,
                "Lab results has 1 message after restart");
    test_assert(hl7sql_store_size(pharm_store) == 1,
                "Pharmacy has 1 message after restart");

    /* === PHASE 3: Drop a channel and restart again === */
    printf("\n--- Phase 3: Dropping channel and restarting ---\n");

    ret = hl7sql_queue_manager_drop_queue(mgr, "pharmacy");
    test_assert(ret == 0, "Dropped pharmacy queue");

    hl7sql_queue_manager_destroy(mgr);
    persistence_cleanup();

    /* Restart again */
    persistence_init(pconfig);
    mgr = hl7sql_queue_manager_create();
    hl7sql_queue_manager_set_persistence(mgr, pconfig);

    loaded = hl7sql_queue_manager_load_registry(mgr);
    test_assert(loaded == 2, "Loaded 2 queues after drop");

    test_assert(hl7sql_queue_manager_has_queue(mgr, "admissions"),
                "Admissions still exists");
    test_assert(hl7sql_queue_manager_has_queue(mgr, "lab_results"),
                "Lab results still exists");
    test_assert(!hl7sql_queue_manager_has_queue(mgr, "pharmacy"),
                "Pharmacy not restored (was dropped)");

    /* === PHASE 4: Create new channel after restart === */
    printf("\n--- Phase 4: Creating new channel after restart ---\n");

    ret = hl7sql_queue_manager_create_queue(mgr, "radiology");
    test_assert(ret == 0, "Created radiology queue after restart");

    /* Final restart to verify new channel persists */
    hl7sql_queue_manager_destroy(mgr);
    persistence_cleanup();

    persistence_init(pconfig);
    mgr = hl7sql_queue_manager_create();
    hl7sql_queue_manager_set_persistence(mgr, pconfig);

    loaded = hl7sql_queue_manager_load_registry(mgr);
    test_assert(loaded == 3, "Loaded 3 queues in final restart");

    test_assert(hl7sql_queue_manager_has_queue(mgr, "radiology"),
                "New radiology queue persisted");

    /* Cleanup */
    hl7sql_queue_manager_destroy(mgr);
    persistence_cleanup();
    free(pconfig->data_dir);
    free(pconfig);
    cleanup_test_data();
    logger_cleanup();

    printf("\n=== Test Results ===\n");
    printf("Passed: %d/%d\n", test_passed, test_count);

    return (test_passed == test_count) ? 0 : 1;
}
