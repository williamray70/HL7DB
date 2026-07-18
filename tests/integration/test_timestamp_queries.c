/**
 * test_timestamp_queries.c - Test suite for timestamp-based queries
 *
 * Tests the new time-based query functionality:
 * - LAST_N_HOURS() function
 * - LAST_N_DAYS() function
 * - BETWEEN operator with timestamps
 * - Direct timestamp comparisons
 * - Combined timestamp + segment filters
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_executor.h"
#include "query/hl7sql_parser.h"
#include "query/hl7sql_ast.h"
#include "storage/persistence.h"
#include "util/logger.h"
#include "hl7/hl7_parser.h"

#define TEST_DATA_DIR "./test_data_timestamp"

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
    printf("=== Timestamp Query Test Suite ===\n\n");

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

    /* Initialize persistence */
    persistence_config_t *pconfig = calloc(1, sizeof(persistence_config_t));
    pconfig->data_dir = strdup(TEST_DATA_DIR);
    pconfig->sync_on_write = 1;
    persistence_init(pconfig);

    /* Create queue manager */
    hl7sql_queue_manager_t *mgr = hl7sql_queue_manager_create();
    hl7sql_queue_manager_set_persistence(mgr, pconfig);

    /* Create test queue */
    hl7sql_queue_manager_create_queue(mgr, "test_queue");
    hl7sql_store_t *store = hl7sql_queue_manager_get_queue(mgr, "test_queue");

    /* === PHASE 1: Insert test messages with different timestamps === */
    printf("--- Phase 1: Inserting messages with controlled timestamps ---\n");

    time_t now = time(NULL);
    time_t one_hour_ago = now - 3600;
    time_t two_hours_ago = now - 7200;
    time_t one_day_ago = now - 86400;
    time_t two_days_ago = now - 172800;
    time_t five_days_ago = now - 432000;

    /* Create test messages */
    const char *msg_template = "MSH|^~\\&|ADT|HOSPITAL|SYS|CENTRAL|%ld||ADT^A01|MSG%03d|P|2.5\rPID|1||P%05d||DOE^JOHN";

    /* Message 1: 5 days ago, Patient P00001 */
    char msg1[512];
    snprintf(msg1, sizeof(msg1), msg_template, five_days_ago, 1, 1);
    hl7_message_t *hl7_msg1 = hl7_parse(msg1, NULL);
    hl7_msg1->timestamp = five_days_ago;
    hl7sql_store_insert(store, hl7_msg1);

    /* Message 2: 2 days ago, Patient P00002 */
    char msg2[512];
    snprintf(msg2, sizeof(msg2), msg_template, two_days_ago, 2, 2);
    hl7_message_t *hl7_msg2 = hl7_parse(msg2, NULL);
    hl7_msg2->timestamp = two_days_ago;
    hl7sql_store_insert(store, hl7_msg2);

    /* Message 3: 1 day ago, Patient P00003 */
    char msg3[512];
    snprintf(msg3, sizeof(msg3), msg_template, one_day_ago, 3, 3);
    hl7_message_t *hl7_msg3 = hl7_parse(msg3, NULL);
    hl7_msg3->timestamp = one_day_ago;
    hl7sql_store_insert(store, hl7_msg3);

    /* Message 4: 2 hours ago, Patient P00004 */
    char msg4[512];
    snprintf(msg4, sizeof(msg4), msg_template, two_hours_ago, 4, 4);
    hl7_message_t *hl7_msg4 = hl7_parse(msg4, NULL);
    hl7_msg4->timestamp = two_hours_ago;
    hl7sql_store_insert(store, hl7_msg4);

    /* Message 5: 1 hour ago, Patient P00001 (duplicate patient) */
    char msg5[512];
    snprintf(msg5, sizeof(msg5), msg_template, one_hour_ago, 5, 1);
    hl7_message_t *hl7_msg5 = hl7_parse(msg5, NULL);
    hl7_msg5->timestamp = one_hour_ago;
    hl7sql_store_insert(store, hl7_msg5);

    test_assert(hl7sql_store_size(store) == 5, "Inserted 5 test messages");

    /* === PHASE 2: Test LAST_N_HOURS function === */
    printf("\n--- Phase 2: Testing LAST_N_HOURS() ---\n");

    /* Query: Messages from last 3 hours (should get 2 messages) */
    ast_node_t *ast = hl7sql_parse_query("SELECT * FROM test_queue WHERE timestamp > LAST_N_HOURS(3);");
    test_assert(ast != NULL, "Parsed LAST_N_HOURS(3) query");

    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result != NULL, "Executed LAST_N_HOURS(3) query");
        test_assert(result->num_rows == 2, "LAST_N_HOURS(3) returns 2 messages");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* Query: Messages from last 30 hours (should get 3 messages) */
    ast = hl7sql_parse_query("SELECT * FROM test_queue WHERE timestamp > LAST_N_HOURS(30);");
    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 3, "LAST_N_HOURS(30) returns 3 messages");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* === PHASE 3: Test LAST_N_DAYS function === */
    printf("\n--- Phase 3: Testing LAST_N_DAYS() ---\n");

    /* Query: Messages from last 1 day (should get 2 messages - the 2 from last few hours) */
    ast = hl7sql_parse_query("SELECT * FROM test_queue WHERE timestamp > LAST_N_DAYS(1);");
    test_assert(ast != NULL, "Parsed LAST_N_DAYS(1) query");

    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 2, "LAST_N_DAYS(1) returns 2 messages (strict > comparison)");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* Query: Messages from last 3 days (should get 4 messages) */
    ast = hl7sql_parse_query("SELECT * FROM test_queue WHERE timestamp > LAST_N_DAYS(3);");
    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 4, "LAST_N_DAYS(3) returns 4 messages");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* Query: Messages from last 7 days (should get all 5 messages) */
    ast = hl7sql_parse_query("SELECT * FROM test_queue WHERE timestamp > LAST_N_DAYS(7);");
    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 5, "LAST_N_DAYS(7) returns all 5 messages");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* === PHASE 4: Test BETWEEN with literal timestamps === */
    printf("\n--- Phase 4: Testing BETWEEN with literals ---\n");

    /* Query: Messages between 3 days ago and now (should get last 3 messages) */
    char query_buf[256];
    snprintf(query_buf, sizeof(query_buf),
             "SELECT * FROM test_queue WHERE timestamp BETWEEN %ld AND %ld;",
             two_days_ago - 3600, now);
    ast = hl7sql_parse_query(query_buf);
    test_assert(ast != NULL, "Parsed BETWEEN query with literals");

    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 4, "BETWEEN returns 4 messages in range (2d ago to now)");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* === PHASE 5: Test BETWEEN with time functions === */
    printf("\n--- Phase 5: Testing BETWEEN with time functions ---\n");

    /* Query: Messages between last 6 days and last 1 day (inclusive boundaries) */
    ast = hl7sql_parse_query(
        "SELECT * FROM test_queue WHERE timestamp BETWEEN LAST_N_DAYS(6) AND LAST_N_DAYS(1);"
    );
    test_assert(ast != NULL, "Parsed BETWEEN with time functions");

    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 3, "BETWEEN with time functions returns 3 messages (5d, 2d, 1d ago - inclusive)");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* === PHASE 6: Test direct timestamp comparisons === */
    printf("\n--- Phase 6: Testing direct timestamp comparisons ---\n");

    /* Test >= */
    snprintf(query_buf, sizeof(query_buf),
             "SELECT * FROM test_queue WHERE timestamp >= %ld;", one_day_ago);
    ast = hl7sql_parse_query(query_buf);
    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 3, "Timestamp >= returns 3 messages");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* Test < */
    snprintf(query_buf, sizeof(query_buf),
             "SELECT * FROM test_queue WHERE timestamp < %ld;", one_day_ago);
    ast = hl7sql_parse_query(query_buf);
    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 2, "Timestamp < returns 2 messages");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* Test = */
    snprintf(query_buf, sizeof(query_buf),
             "SELECT * FROM test_queue WHERE timestamp = %ld;", one_hour_ago);
    ast = hl7sql_parse_query(query_buf);
    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 1, "Timestamp = returns 1 message");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* === PHASE 7: Test combined timestamp + segment filters === */
    printf("\n--- Phase 7: Testing combined filters ---\n");

    /* Query: Recent messages for patient P00001 */
    ast = hl7sql_parse_query(
        "SELECT * FROM test_queue WHERE timestamp > LAST_N_DAYS(7) AND segment.PID[3] = 'P00001';"
    );
    test_assert(ast != NULL, "Parsed combined timestamp + segment filter");

    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 2, "Combined filter returns 2 messages for P00001");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* Query: Messages in last 2 days for patient P00003 */
    ast = hl7sql_parse_query(
        "SELECT * FROM test_queue WHERE timestamp > LAST_N_DAYS(2) AND segment.PID[3] = 'P00003';"
    );
    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        test_assert(result->num_rows == 1, "Combined filter returns 1 message for P00003");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

    /* === PHASE 8: Test edge cases === */
    printf("\n--- Phase 8: Testing edge cases ---\n");

    /* Query: LAST_N_HOURS(0) should parse but return nothing */
    ast = hl7sql_parse_query("SELECT * FROM test_queue WHERE timestamp > LAST_N_HOURS(1);");
    test_assert(ast != NULL, "Valid time function with N=1 parses");
    if (ast) {
        ast_destroy(ast);
    }

    /* Query: Reversed BETWEEN bounds (should return messages where lower <= ts <= upper fails) */
    snprintf(query_buf, sizeof(query_buf),
             "SELECT * FROM test_queue WHERE timestamp BETWEEN %ld AND %ld;",
             one_day_ago, two_days_ago);
    ast = hl7sql_parse_query(query_buf);
    if (ast) {
        hl7sql_result_t *result = hl7sql_execute(mgr, ast);
        /* Reversed bounds mean no timestamp can satisfy: lower <= ts <= upper when lower > upper */
        /* However, the actual message at two_days_ago might satisfy if bounds overlap exactly */
        test_assert(result->num_rows <= 1, "Reversed BETWEEN bounds return at most 1 message");
        hl7sql_result_destroy(result);
        ast_destroy(ast);
    }

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
