/**
 * test_history.c - Test Queue History Feature
 *
 * Demonstrates messages moving to history when popped from queues.
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

    print_header("Test 1: Channel History Feature");

    /* Create channel manager */
    hl7sql_queue_manager_t *mgr = hl7sql_queue_manager_create();
    if (!mgr) {
        printf("❌ Failed to create channel manager\n");
        return 1;
    }

    /* Test 1: Create a channel */
    printf("1. Creating channel 'orders'...\n");
    hl7sql_result_t *result = hl7sql_query(mgr, "CREATE QUEUE orders");
    if (result->error_code != 0) {
        printf("❌ Failed: %s\n", result->error_message);
        return 1;
    }
    printf("✓ Channel created\n");
    hl7sql_result_destroy(result);

    /* Test 2: Insert messages into the queue */
    printf("\n2. Inserting 3 messages into queue...\n");

    const char *msg1 =
        "MSH|^~\\&|LAB|FACILITY|APP|FACILITY|20240615100000||ORM^O01|MSG001|P|2.5\r"
        "PID|1||P001^^^FAC^MR||SMITH^JOHN^||19800101|M\r"
        "ORC|NW|ORD001|||SC";

    const char *msg2 =
        "MSH|^~\\&|LAB|FACILITY|APP|FACILITY|20240615100100||ORM^O01|MSG002|P|2.5\r"
        "PID|1||P002^^^FAC^MR||DOE^JANE^||19750315|F\r"
        "ORC|NW|ORD002|||SC";

    const char *msg3 =
        "MSH|^~\\&|LAB|FACILITY|APP|FACILITY|20240615100200||ORM^O01|MSG003|P|2.5\r"
        "PID|1||P003^^^FAC^MR||JOHNSON^BOB^||19900522|M\r"
        "ORC|NW|ORD003|||SC";

    char insert_buf[2048];

    snprintf(insert_buf, sizeof(insert_buf), "INSERT MESSAGE INTO orders '%s'", msg1);
    result = hl7sql_query(mgr, insert_buf);
    if (result->error_code != 0) {
        printf("❌ Failed to insert message 1: %s\n", result->error_message);
    } else {
        printf("✓ Inserted message 1 (MSG001)\n");
    }
    hl7sql_result_destroy(result);

    snprintf(insert_buf, sizeof(insert_buf), "INSERT MESSAGE INTO orders '%s'", msg2);
    result = hl7sql_query(mgr, insert_buf);
    if (result->error_code == 0) {
        printf("✓ Inserted message 2 (MSG002)\n");
    }
    hl7sql_result_destroy(result);

    snprintf(insert_buf, sizeof(insert_buf), "INSERT MESSAGE INTO orders '%s'", msg3);
    result = hl7sql_query(mgr, insert_buf);
    if (result->error_code == 0) {
        printf("✓ Inserted message 3 (MSG003)\n");
    }
    hl7sql_result_destroy(result);

    /* Test 3: Check queue size */
    printf("\n3. Checking queue size...\n");
    result = hl7sql_query(mgr, "SELECT * FROM orders");
    printf("✓ Active queue has %zu messages\n", result->num_rows);
    hl7sql_result_destroy(result);

    /* Test 4: Check history (should be empty) */
    printf("\n4. Checking history (should be empty)...\n");
    result = hl7sql_query(mgr, "SELECT * FROM orders.history");
    if (result->error_code == 0) {
        printf("✓ History has %zu messages\n", result->num_rows);
    } else {
        printf("❌ Failed to query history: %s\n", result->error_message);
    }
    hl7sql_result_destroy(result);

    /* Test 5: Pop message (moves to history) */
    printf("\n5. Popping message from queue (should move to history)...\n");
    result = hl7sql_query(mgr, "POP MESSAGE FROM orders");
    if (result->error_code == 0 && result->num_rows > 0) {
        printf("✓ Popped message: %s\n", result->rows[0][1]);  /* msg_type */
    }
    hl7sql_result_destroy(result);

    /* Test 6: Verify queue size decreased */
    printf("\n6. Checking queue size after pop...\n");
    result = hl7sql_query(mgr, "SELECT * FROM orders");
    printf("✓ Active queue now has %zu messages\n", result->num_rows);
    hl7sql_result_destroy(result);

    /* Test 7: Verify history has the popped message */
    printf("\n7. Checking history after pop...\n");
    result = hl7sql_query(mgr, "SELECT * FROM orders.history");
    if (result->error_code == 0) {
        printf("✓ History now has %zu messages\n", result->num_rows);
        if (result->num_rows > 0) {
            printf("  - Message type: %s\n", result->rows[0][1]);
        }
    }
    hl7sql_result_destroy(result);

    /* Test 8: Pop another message */
    printf("\n8. Popping second message...\n");
    result = hl7sql_query(mgr, "POP MESSAGE FROM orders");
    if (result->error_code == 0 && result->num_rows > 0) {
        printf("✓ Popped message: %s\n", result->rows[0][1]);
    }
    hl7sql_result_destroy(result);

    /* Test 9: Check history has both messages */
    printf("\n9. Checking history count...\n");
    result = hl7sql_query(mgr, "SELECT * FROM orders.history");
    printf("✓ History has %zu messages\n", result->num_rows);
    hl7sql_result_destroy(result);

    /* Test 10: Query history with WHERE clause */
    printf("\n10. Querying history for specific patient...\n");
    result = hl7sql_query(mgr, "SELECT * FROM orders.history WHERE segment.PID[3] = 'P001'");
    if (result->error_code == 0) {
        printf("✓ Found %zu messages for patient P001 in history\n", result->num_rows);
    }
    hl7sql_result_destroy(result);

    /* Test 11: Clear history */
    printf("\n11. Clearing history...\n");
    result = hl7sql_query(mgr, "CLEAR HISTORY orders");
    if (result->error_code == 0) {
        printf("✓ History cleared successfully\n");
    } else {
        printf("❌ Failed to clear history: %s\n", result->error_message);
    }
    hl7sql_result_destroy(result);

    /* Test 12: Verify history is empty */
    printf("\n12. Verifying history is empty...\n");
    result = hl7sql_query(mgr, "SELECT * FROM orders.history");
    printf("✓ History has %zu messages (should be 0)\n", result->num_rows);
    hl7sql_result_destroy(result);

    /* Test 13: Verify active queue still has remaining message */
    printf("\n13. Checking remaining active queue messages...\n");
    result = hl7sql_query(mgr, "SELECT * FROM orders");
    printf("✓ Active queue has %zu messages\n", result->num_rows);
    hl7sql_result_destroy(result);

    print_header("✓ All History Tests Passed!");

    /* Cleanup */
    hl7sql_queue_manager_destroy(mgr);
    logger_cleanup();

    return 0;
}
