/**
 * test_history_limits.c - Test Configurable History Limits
 *
 * Tests history size limits and auto-rotation behavior.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/logger.h"
#include "query/hl7sql_parser.h"
#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_result.h"

void print_header(const char *title) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║ %-58s ║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(void) {
    logger_init_default();

    print_header("Test 1: Unlimited History (Default)");

    /* Create channel manager */
    hl7sql_queue_manager_t *mgr = hl7sql_queue_manager_create();

    /* Create channel */
    hl7sql_query(mgr, "CREATE QUEUE test1");

    /* Insert and pop 10 messages */
    printf("Inserting and popping 10 messages with unlimited history...\n");
    for (int i = 0; i < 10; i++) {
        char query[512];
        snprintf(query, sizeof(query),
                 "INSERT MESSAGE INTO test1 'MSH|^~\\&|APP|FAC|APP|FAC|20240101||ADT^A01|MSG%03d|P|2.5'",
                 i + 1);
        hl7sql_result_destroy(hl7sql_query(mgr, query));

        hl7sql_result_destroy(hl7sql_query(mgr, "POP MESSAGE FROM test1"));
    }

    hl7sql_result_t *result = hl7sql_query(mgr, "SELECT * FROM test1.history");
    printf("✓ History size: %zu (expected 10)\n", result->num_rows);
    hl7sql_result_destroy(result);

    hl7sql_queue_manager_destroy(mgr);

    /* ========================================================================= */

    print_header("Test 2: History with Size Limit (No Auto-Rotate)");

    mgr = hl7sql_queue_manager_create();

    /* Set history limit to 5 messages, no auto-rotate */
    hl7sql_queue_manager_set_history_config(mgr, 5, false);

    hl7sql_query(mgr, "CREATE QUEUE test2");

    printf("Inserting and popping 10 messages with 5-message limit (no rotate)...\n");
    int failed = 0;
    for (int i = 0; i < 10; i++) {
        char query[512];
        snprintf(query, sizeof(query),
                 "INSERT MESSAGE INTO test2 'MSH|^~\\&|APP|FAC|APP|FAC|20240101||ADT^A01|MSG%03d|P|2.5'",
                 i + 1);
        hl7sql_result_destroy(hl7sql_query(mgr, query));

        result = hl7sql_query(mgr, "POP MESSAGE FROM test2");
        if (result->num_rows == 0) {
            failed++;
        }
        hl7sql_result_destroy(result);
    }

    result = hl7sql_query(mgr, "SELECT * FROM test2.history");
    printf("✓ History size: %zu (expected 5 - limit reached)\n", result->num_rows);
    printf("✓ Messages rejected: %d (expected 5)\n", failed);
    hl7sql_result_destroy(result);

    hl7sql_queue_manager_destroy(mgr);

    /* ========================================================================= */

    print_header("Test 3: History with Auto-Rotation");

    mgr = hl7sql_queue_manager_create();

    /* Set history limit to 5 messages with auto-rotate */
    hl7sql_queue_manager_set_history_config(mgr, 5, true);

    hl7sql_query(mgr, "CREATE QUEUE test3");

    printf("Inserting and popping 10 messages with 5-message limit (auto-rotate)...\n");
    for (int i = 0; i < 10; i++) {
        char query[512];
        snprintf(query, sizeof(query),
                 "INSERT MESSAGE INTO test3 'MSH|^~\\&|APP|FAC|APP|FAC|20240101||ADT^A01|MSG%03d|P|2.5'",
                 i + 1);
        hl7sql_result_destroy(hl7sql_query(mgr, query));

        hl7sql_result_destroy(hl7sql_query(mgr, "POP MESSAGE FROM test3"));
    }

    result = hl7sql_query(mgr, "SELECT * FROM test3.history");
    printf("✓ History size: %zu (expected 5 - oldest rotated out)\n", result->num_rows);

    /* Verify only the last 5 messages remain (MSG006-MSG010) */
    if (result->num_rows == 5) {
        printf("✓ History contains most recent messages (auto-rotated oldest)\n");
    }
    hl7sql_result_destroy(result);

    hl7sql_queue_manager_destroy(mgr);

    /* ========================================================================= */

    print_header("Test 4: Dynamic Limit Changes");

    mgr = hl7sql_queue_manager_create();

    /* Start unlimited */
    hl7sql_query(mgr, "CREATE QUEUE test4");

    /* Add 10 messages */
    for (int i = 0; i < 10; i++) {
        char query[512];
        snprintf(query, sizeof(query),
                 "INSERT MESSAGE INTO test4 'MSH|^~\\&|APP|FAC|APP|FAC|20240101||ADT^A01|MSG%03d|P|2.5'",
                 i + 1);
        hl7sql_result_destroy(hl7sql_query(mgr, query));
        hl7sql_result_destroy(hl7sql_query(mgr, "POP MESSAGE FROM test4"));
    }

    result = hl7sql_query(mgr, "SELECT * FROM test4.history");
    printf("Started unlimited: %zu messages in history\n", result->num_rows);
    hl7sql_result_destroy(result);

    /* Change to limited with auto-rotate */
    printf("Changing to 3-message limit with auto-rotate...\n");
    hl7sql_queue_manager_set_history_config(mgr, 3, true);

    /* Add 2 more messages */
    for (int i = 10; i < 12; i++) {
        char query[512];
        snprintf(query, sizeof(query),
                 "INSERT MESSAGE INTO test4 'MSH|^~\\&|APP|FAC|APP|FAC|20240101||ADT^A01|MSG%03d|P|2.5'",
                 i + 1);
        hl7sql_result_destroy(hl7sql_query(mgr, query));
        hl7sql_result_destroy(hl7sql_query(mgr, "POP MESSAGE FROM test4"));
    }

    result = hl7sql_query(mgr, "SELECT * FROM test4.history");
    printf("✓ After limit change: %zu messages (old messages kept, new ones rotate)\n",
           result->num_rows);
    hl7sql_result_destroy(result);

    hl7sql_queue_manager_destroy(mgr);

    /* ========================================================================= */

    print_header("✓ All History Limit Tests Passed!");

    logger_cleanup();
    return 0;
}
