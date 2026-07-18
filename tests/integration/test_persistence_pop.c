/**
 * test_persistence_pop.c - Test persistence rewrite on POP operations
 *
 * Verifies that when messages are popped from a queue, the persistence
 * file is correctly rewritten to exclude the popped messages.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_store.h"
#include "storage/persistence.h"
#include "util/logger.h"
#include "util/list.h"
#include "hl7/hl7_parser.h"

#define TEST_DATA_DIR "./test_data_pop"
#define TEST_QUEUE "test_queue"

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
    (void)ret;  /* Ignore return value - cleanup is best-effort */
}

static size_t count_lines_in_file(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    size_t count = 0;
    char *line = NULL;
    size_t line_size = 0;
    while (getline(&line, &line_size, f) != -1) {
        count++;
    }
    free(line);
    fclose(f);
    return count;
}

int main(void) {
    printf("=== Testing Persistence Rewrite on POP Operations ===\n\n");

    /* Initialize logger */
    logger_config_t log_config = {
        .min_level = LOG_DEBUG,
        .file = NULL,
        .use_color = 1,
        .include_timestamp = 1,
        .include_level = 1,
        .include_location = 0
    };
    logger_init(&log_config);

    /* Clean up any previous test data */
    cleanup_test_data();

    /* Initialize persistence configuration */
    persistence_config_t *pconfig = calloc(1, sizeof(persistence_config_t));
    pconfig->data_dir = strdup(TEST_DATA_DIR);
    pconfig->sync_on_write = 1;

    int ret = persistence_init(pconfig);
    test_assert(ret == 0, "Persistence initialization succeeded");

    /* Create queue manager */
    hl7sql_queue_manager_t *mgr = hl7sql_queue_manager_create();
    test_assert(mgr != NULL, "Queue manager created");

    /* Set persistence configuration */
    ret = hl7sql_queue_manager_set_persistence(mgr, pconfig);
    test_assert(ret == 0, "Queue manager persistence configuration set");

    /* Create test queue */
    ret = hl7sql_queue_manager_create_queue(mgr, TEST_QUEUE);
    test_assert(ret == 0, "Test queue created");

    /* Get the queue store */
    hl7sql_store_t *store = hl7sql_queue_manager_get_queue(mgr, TEST_QUEUE);
    test_assert(store != NULL, "Queue store retrieved");

    /* Insert three test messages */
    const char *msg1_raw = "MSH|^~\\&|SENDER|FACILITY|RECEIVER|DESTINATION|20240101120000||ADT^A01|MSG001|P|2.5\rPID|1||12345||DOE^JOHN";
    const char *msg2_raw = "MSH|^~\\&|SENDER|FACILITY|RECEIVER|DESTINATION|20240101120001||ADT^A01|MSG002|P|2.5\rPID|1||12346||SMITH^JANE";
    const char *msg3_raw = "MSH|^~\\&|SENDER|FACILITY|RECEIVER|DESTINATION|20240101120002||ADT^A01|MSG003|P|2.5\rPID|1||12347||JONES^BOB";

    hl7_message_t *msg1 = hl7_parse(msg1_raw, NULL);
    hl7_message_t *msg2 = hl7_parse(msg2_raw, NULL);
    hl7_message_t *msg3 = hl7_parse(msg3_raw, NULL);

    test_assert(msg1 != NULL, "Message 1 parsed");
    test_assert(msg2 != NULL, "Message 2 parsed");
    test_assert(msg3 != NULL, "Message 3 parsed");

    ret = hl7sql_store_insert(store, msg1);
    test_assert(ret == 0, "Message 1 inserted");

    ret = hl7sql_store_insert(store, msg2);
    test_assert(ret == 0, "Message 2 inserted");

    ret = hl7sql_store_insert(store, msg3);
    test_assert(ret == 0, "Message 3 inserted");

    /* Build persistence file path for later checks */
    char persistence_file[512];
    snprintf(persistence_file, sizeof(persistence_file), "%s/channels/%s/messages.dat",
             TEST_DATA_DIR, TEST_QUEUE);
    size_t line_count = 0;

    /* Note: Direct store inserts don't trigger persistence - only INSERT queries do.
     * The POP operation will create the persistence file via rewrite. */

    /* Simulate POP: remove message from store and rewrite persistence */
    hl7sql_store_write_lock(store);
    list_t *messages = hl7sql_store_get_all(store);
    hl7_message_t *popped_msg = list_remove_first(messages);
    hl7sql_store_write_unlock(store);

    test_assert(popped_msg != NULL, "Message popped from store");

    /* Move to history */
    ret = hl7sql_queue_manager_move_to_history(mgr, TEST_QUEUE, popped_msg);
    test_assert(ret == 0, "Message moved to history");

    /* Rewrite persistence */
    ret = hl7sql_queue_manager_rewrite_persistence(mgr, TEST_QUEUE);
    test_assert(ret == 0, "Persistence rewrite after first POP succeeded");

    /* Verify persistence file now has 2 messages */
    line_count = count_lines_in_file(persistence_file);
    test_assert(line_count == 2, "Persistence file has 2 messages after first POP");

    /* Verify queue still has 2 messages in memory */
    size_t queue_size = hl7sql_store_size(store);
    test_assert(queue_size == 2, "Queue has 2 messages in memory after first POP");

    /* Pop another message */
    hl7sql_store_write_lock(store);
    messages = hl7sql_store_get_all(store);
    popped_msg = list_remove_first(messages);
    hl7sql_store_write_unlock(store);

    test_assert(popped_msg != NULL, "Second message popped from store");

    ret = hl7sql_queue_manager_move_to_history(mgr, TEST_QUEUE, popped_msg);
    test_assert(ret == 0, "Second message moved to history");

    ret = hl7sql_queue_manager_rewrite_persistence(mgr, TEST_QUEUE);
    test_assert(ret == 0, "Persistence rewrite after second POP succeeded");

    /* Verify persistence file now has 1 message */
    line_count = count_lines_in_file(persistence_file);
    test_assert(line_count == 1, "Persistence file has 1 message after second POP");

    /* Verify queue has 1 message in memory */
    queue_size = hl7sql_store_size(store);
    test_assert(queue_size == 1, "Queue has 1 message in memory after second POP");

    /* Pop the last message */
    hl7sql_store_write_lock(store);
    messages = hl7sql_store_get_all(store);
    popped_msg = list_remove_first(messages);
    hl7sql_store_write_unlock(store);

    test_assert(popped_msg != NULL, "Third message popped from store");

    ret = hl7sql_queue_manager_move_to_history(mgr, TEST_QUEUE, popped_msg);
    test_assert(ret == 0, "Third message moved to history");

    ret = hl7sql_queue_manager_rewrite_persistence(mgr, TEST_QUEUE);
    test_assert(ret == 0, "Persistence rewrite after third POP succeeded");

    /* Verify persistence file is now empty */
    line_count = count_lines_in_file(persistence_file);
    test_assert(line_count == 0, "Persistence file is empty after all messages popped");

    /* Verify queue is empty in memory */
    queue_size = hl7sql_store_size(store);
    test_assert(queue_size == 0, "Queue is empty in memory after all messages popped");

    /* === Test persistence reload === */
    printf("\n--- Testing persistence reload after POP ---\n");

    /* Insert 3 new messages */
    hl7_message_t *msg4 = hl7_parse(msg1_raw, NULL);
    hl7_message_t *msg5 = hl7_parse(msg2_raw, NULL);
    hl7_message_t *msg6 = hl7_parse(msg3_raw, NULL);

    hl7sql_store_insert(store, msg4);
    hl7sql_store_insert(store, msg5);
    hl7sql_store_insert(store, msg6);

    /* Note: These direct inserts won't be persisted until the next rewrite */

    /* Pop one message */
    hl7sql_store_write_lock(store);
    messages = hl7sql_store_get_all(store);
    popped_msg = list_remove_first(messages);
    hl7sql_store_write_unlock(store);

    hl7sql_queue_manager_move_to_history(mgr, TEST_QUEUE, popped_msg);
    hl7sql_queue_manager_rewrite_persistence(mgr, TEST_QUEUE);

    line_count = count_lines_in_file(persistence_file);
    test_assert(line_count == 2, "Persistence file has 2 messages after POP");

    /* Destroy and recreate queue manager (simulating restart) */
    hl7sql_queue_manager_destroy(mgr);

    /* Create new queue manager and reload from persistence */
    mgr = hl7sql_queue_manager_create();
    hl7sql_queue_manager_set_persistence(mgr, pconfig);
    hl7sql_queue_manager_create_queue(mgr, TEST_QUEUE);

    store = hl7sql_queue_manager_get_queue(mgr, TEST_QUEUE);
    queue_size = hl7sql_store_size(store);
    test_assert(queue_size == 2, "Queue reloaded with 2 messages (popped message did not reappear)");

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
