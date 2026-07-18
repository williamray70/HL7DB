/**
 * test_channel_registry.c - Test channel registry persistence
 *
 * Verifies that channel names are persisted in the registry file
 * and restored on server restart.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

#include "query/hl7sql_queue_manager.h"
#include "storage/persistence.h"
#include "util/logger.h"

#define TEST_DATA_DIR "./test_data_registry"

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

static int file_contains_line(const char *filepath, const char *search) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    int found = 0;
    char *line = NULL;
    size_t line_size = 0;
    while (getline(&line, &line_size, f) != -1) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (strcmp(line, search) == 0) {
            found = 1;
            break;
        }
    }
    free(line);
    fclose(f);
    return found;
}

int main(void) {
    printf("=== Testing Channel Registry Persistence ===\n\n");

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

    /* Registry file path */
    char registry_file[512];
    snprintf(registry_file, sizeof(registry_file), "%s/channels/channels.dat",
             TEST_DATA_DIR);

    /* Create three test queues */
    ret = hl7sql_queue_manager_create_queue(mgr, "queue1");
    test_assert(ret == 0, "Queue 'queue1' created");

    ret = hl7sql_queue_manager_create_queue(mgr, "queue2");
    test_assert(ret == 0, "Queue 'queue2' created");

    ret = hl7sql_queue_manager_create_queue(mgr, "queue3");
    test_assert(ret == 0, "Queue 'queue3' created");

    /* Verify registry file has 3 channels */
    size_t line_count = count_lines_in_file(registry_file);
    test_assert(line_count == 3, "Registry file has 3 channel names");

    /* Verify each channel is in the registry */
    test_assert(file_contains_line(registry_file, "queue1"),
                "Registry contains 'queue1'");
    test_assert(file_contains_line(registry_file, "queue2"),
                "Registry contains 'queue2'");
    test_assert(file_contains_line(registry_file, "queue3"),
                "Registry contains 'queue3'");

    /* Drop one queue */
    ret = hl7sql_queue_manager_drop_queue(mgr, "queue2");
    test_assert(ret == 0, "Queue 'queue2' dropped");

    /* Verify registry file now has 2 channels */
    line_count = count_lines_in_file(registry_file);
    test_assert(line_count == 2, "Registry file has 2 channel names after drop");

    /* Verify queue2 is no longer in registry */
    test_assert(!file_contains_line(registry_file, "queue2"),
                "Registry does not contain 'queue2' after drop");
    test_assert(file_contains_line(registry_file, "queue1"),
                "Registry still contains 'queue1'");
    test_assert(file_contains_line(registry_file, "queue3"),
                "Registry still contains 'queue3'");

    /* Destroy queue manager (simulating server shutdown) */
    hl7sql_queue_manager_destroy(mgr);

    printf("\n--- Testing Registry Load on Startup ---\n");

    /* Create new queue manager (simulating server restart) */
    mgr = hl7sql_queue_manager_create();
    test_assert(mgr != NULL, "New queue manager created");

    /* Set persistence configuration */
    ret = hl7sql_queue_manager_set_persistence(mgr, pconfig);
    test_assert(ret == 0, "Persistence set on new manager");

    /* Load registry */
    int loaded = hl7sql_queue_manager_load_registry(mgr);
    test_assert(loaded == 2, "Loaded 2 queues from registry");

    /* Verify queues exist */
    test_assert(hl7sql_queue_manager_has_queue(mgr, "queue1"),
                "Queue 'queue1' restored from registry");
    test_assert(!hl7sql_queue_manager_has_queue(mgr, "queue2"),
                "Queue 'queue2' not restored (was dropped)");
    test_assert(hl7sql_queue_manager_has_queue(mgr, "queue3"),
                "Queue 'queue3' restored from registry");

    /* Verify no duplicate registration on create after load */
    ret = hl7sql_queue_manager_create_queue(mgr, "queue4");
    test_assert(ret == 0, "Queue 'queue4' created after load");

    line_count = count_lines_in_file(registry_file);
    test_assert(line_count == 3, "Registry has 3 channels after adding queue4");

    /* Verify all queues are in registry exactly once */
    FILE *f = fopen(registry_file, "r");
    int queue1_count = 0, queue3_count = 0, queue4_count = 0;
    char *line = NULL;
    size_t line_size = 0;
    while (getline(&line, &line_size, f) != -1) {
        if (strstr(line, "queue1")) queue1_count++;
        if (strstr(line, "queue3")) queue3_count++;
        if (strstr(line, "queue4")) queue4_count++;
    }
    free(line);
    fclose(f);

    test_assert(queue1_count == 1, "Queue1 appears exactly once in registry");
    test_assert(queue3_count == 1, "Queue3 appears exactly once in registry");
    test_assert(queue4_count == 1, "Queue4 appears exactly once in registry");

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
