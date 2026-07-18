/**
 * test_queue_tester.c - Interactive Queue Tester Application
 *
 * Comprehensive tool for viewing, querying, and inserting into HL7 message queues.
 * Provides an interactive menu-driven interface for testing queue functionality.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_executor.h"
#include "query/hl7sql_parser.h"
#include "query/hl7sql_result.h"
#include "hl7/hl7_message.h"
#include "hl7/hl7_parser.h"
#include "util/logger.h"

/* ====================================================================
 * Sample HL7 Messages
 * ==================================================================== */

/* Sample ADT^A01 (Patient Admission) */
static const char *SAMPLE_ADT_A01 =
    "MSH|^~\\&|SENDING_APP|SENDING_FAC|RECV_APP|RECV_FAC|20260618120000||ADT^A01|MSG00001|P|2.5\r"
    "EVN|A01|20260618120000\r"
    "PID|1||12345^^^MRN||DOE^JOHN^A||19800101|M||W|123 MAIN ST^^ANYTOWN^CA^12345||555-1234|||M\r"
    "PV1|1|I|ICU^101^1|||123^SMITH^JOHN|||MED||||1|||123^SMITH^JOHN|INP|12345|||||||||||||||||||||||||20260618120000\r";

/* Sample ORU^R01 (Lab Result) */
static const char *SAMPLE_ORU_R01 =
    "MSH|^~\\&|LAB_SYS|LAB_FAC|RECV_APP|RECV_FAC|20260618130000||ORU^R01|MSG00002|P|2.5\r"
    "PID|1||67890^^^MRN||SMITH^JANE^B||19900215|F||B|456 ELM ST^^OTHERTOWN^NY^67890||555-5678|||F\r"
    "OBR|1|ORD12345|LAB54321|CBC^COMPLETE BLOOD COUNT|||20260618120000\r"
    "OBX|1|NM|WBC^White Blood Count||7.5|10*3/uL|4.5-11.0|N|||F\r"
    "OBX|2|NM|RBC^Red Blood Count||4.8|10*6/uL|4.2-5.9|N|||F\r";

/* Sample ORM^O01 (Order) */
static const char *SAMPLE_ORM_O01 =
    "MSH|^~\\&|ORDER_APP|ORDER_FAC|RECV_APP|RECV_FAC|20260618140000||ORM^O01|MSG00003|P|2.5\r"
    "PID|1||11111^^^MRN||JONES^ROBERT^C||19750520|M||W|789 OAK AVE^^SOMECITY^TX^78901||555-9012|||M\r"
    "ORC|NW|ORD99999||||||20260618140000\r"
    "OBR|1|ORD99999||XRAY^CHEST X-RAY|||20260618140000\r";

/* ====================================================================
 * Helper Functions
 * ==================================================================== */

/**
 * Print menu header
 */
static void print_header(const char *title) {
    printf("\n");
    printf("================================================================================\n");
    printf("  %s\n", title);
    printf("================================================================================\n");
}

/**
 * Print menu separator
 */
static void print_separator(void) {
    printf("--------------------------------------------------------------------------------\n");
}

/**
 * Wait for user to press Enter
 */
static void wait_for_enter(void) {
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
}

/**
 * Read a line from stdin (removes newline)
 */
static char *read_line(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin) == NULL) {
        return NULL;
    }

    /* Remove trailing newline */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return buffer;
}

/**
 * Execute HL7SQL query and display results
 */
static void execute_and_display(hl7sql_queue_manager_t *mgr, const char *query) {
    printf("\n[EXECUTING] %s\n", query);

    /* Execute the query */
    hl7sql_result_t *result = hl7sql_query(mgr, query);

    if (!result) {
        printf("[ERROR] Query execution failed\n");
        return;
    }

    /* Display results */
    if (result->error_code != 0) {
        printf("[ERROR] %s\n", result->error_message ? result->error_message : "Unknown error");
    } else {
        printf("[SUCCESS] Query executed successfully\n");

        if (result->num_rows > 0) {
            printf("[RESULT] %zu row(s) returned\n\n", result->num_rows);

            /* Print the result set */
            hl7sql_result_print(result);
            printf("\n");
        } else if (result->rows_affected > 0) {
            printf("[RESULT] %zu row(s) affected\n", result->rows_affected);
        }
    }

    hl7sql_result_destroy(result);
}

/**
 * Print all queues
 */
static void list_queues(hl7sql_queue_manager_t *mgr) {
    print_header("LIST ALL QUEUES");

    size_t num_queues = 0;
    char **queue_names = hl7sql_queue_manager_get_all_queues(mgr, &num_queues);

    if (!queue_names || num_queues == 0) {
        printf("No queues exist.\n");
        return;
    }

    printf("Found %zu queue(s):\n\n", num_queues);

    for (size_t i = 0; i < num_queues; i++) {
        const char *name = queue_names[i];

        /* Get queue info */
        hl7sql_store_t *store = hl7sql_queue_manager_get_queue(mgr, name);
        size_t msg_count = store ? hl7sql_store_size(store) : 0;
        size_t history_count = hl7sql_queue_manager_get_history_size(mgr, name);

        printf("  [%zu] %s\n", i + 1, name);
        printf("      Messages: %zu\n", msg_count);
        printf("      History:  %zu\n", history_count);
        printf("\n");
    }

    hl7sql_queue_manager_free_queue_names(queue_names, num_queues);
}

/**
 * Create a new queue
 */
static void create_queue(hl7sql_queue_manager_t *mgr) {
    print_header("CREATE QUEUE");

    char queue_name[256];
    printf("Enter queue name: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query), "CREATE QUEUE %s", queue_name);
    execute_and_display(mgr, query);
}

/**
 * Drop an existing queue
 */
static void drop_queue(hl7sql_queue_manager_t *mgr) {
    print_header("DROP QUEUE");

    list_queues(mgr);

    char queue_name[256];
    printf("Enter queue name to drop: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query), "DROP QUEUE %s", queue_name);
    execute_and_display(mgr, query);
}

/**
 * View messages in a queue
 */
static void view_queue(hl7sql_queue_manager_t *mgr) {
    print_header("VIEW QUEUE MESSAGES");

    list_queues(mgr);

    char queue_name[256];
    printf("Enter queue name: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query), "SELECT * FROM %s", queue_name);
    execute_and_display(mgr, query);
}

/**
 * View history of a queue
 */
static void view_history(hl7sql_queue_manager_t *mgr) {
    print_header("VIEW QUEUE HISTORY");

    list_queues(mgr);

    char queue_name[256];
    printf("Enter queue name: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query), "SELECT * FROM %s.history", queue_name);
    execute_and_display(mgr, query);
}

/**
 * Query messages with WHERE clause
 */
static void query_messages(hl7sql_queue_manager_t *mgr) {
    print_header("QUERY MESSAGES");

    list_queues(mgr);

    char queue_name[256];
    printf("Enter queue name: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    printf("\nExample queries:\n");
    printf("  1. MSH-9 = 'ADT^A01'\n");
    printf("  2. PID-5.1 = 'DOE'\n");
    printf("  3. MSH-3 = 'LAB_SYS'\n");
    printf("  4. TIMESTAMP > 1718704800\n");
    printf("\nEnter WHERE condition (or press Enter for all): ");

    char condition[512];
    if (!read_line(condition, sizeof(condition))) {
        condition[0] = '\0';
    }

    char query[1024];
    if (strlen(condition) > 0) {
        snprintf(query, sizeof(query), "SELECT * FROM %s WHERE %s", queue_name, condition);
    } else {
        snprintf(query, sizeof(query), "SELECT * FROM %s", queue_name);
    }

    execute_and_display(mgr, query);
}

/**
 * Insert a sample message
 */
static void insert_sample_message(hl7sql_queue_manager_t *mgr) {
    print_header("INSERT SAMPLE MESSAGE");

    list_queues(mgr);

    char queue_name[256];
    printf("Enter queue name: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    printf("\nSelect sample message:\n");
    printf("  1. ADT^A01 (Patient Admission)\n");
    printf("  2. ORU^R01 (Lab Result)\n");
    printf("  3. ORM^O01 (Order)\n");
    printf("\nEnter choice (1-3): ");

    char choice[10];
    if (!read_line(choice, sizeof(choice))) {
        printf("Invalid input\n");
        return;
    }

    const char *message = NULL;
    switch (choice[0]) {
        case '1':
            message = SAMPLE_ADT_A01;
            printf("\n[Selected] ADT^A01 (Patient Admission)\n");
            break;
        case '2':
            message = SAMPLE_ORU_R01;
            printf("\n[Selected] ORU^R01 (Lab Result)\n");
            break;
        case '3':
            message = SAMPLE_ORM_O01;
            printf("\n[Selected] ORM^O01 (Order)\n");
            break;
        default:
            printf("Invalid choice\n");
            return;
    }

    printf("\nMessage content:\n%s\n", message);

    /* Build INSERT query */
    char query[4096];
    snprintf(query, sizeof(query), "INSERT INTO %s VALUES '%s'", queue_name, message);
    execute_and_display(mgr, query);
}

/**
 * Insert custom message
 */
static void insert_custom_message(hl7sql_queue_manager_t *mgr) {
    print_header("INSERT CUSTOM MESSAGE");

    list_queues(mgr);

    char queue_name[256];
    printf("Enter queue name: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    printf("\nEnter HL7 message (use \\r for segment separators):\n");
    printf("Example: MSH|^~\\&|APP|FAC|...|\\rPID|1||...\n");
    printf("\nMessage: ");

    char message[4096];
    if (!read_line(message, sizeof(message))) {
        printf("Invalid input\n");
        return;
    }

    /* Build INSERT query */
    char query[8192];
    snprintf(query, sizeof(query), "INSERT INTO %s VALUES '%s'", queue_name, message);
    execute_and_display(mgr, query);
}

/**
 * Pop message from queue
 */
static void pop_message(hl7sql_queue_manager_t *mgr) {
    print_header("POP MESSAGE FROM QUEUE");

    list_queues(mgr);

    char queue_name[256];
    printf("Enter queue name: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    char query[512];
    snprintf(query, sizeof(query), "POP MESSAGE FROM %s", queue_name);
    execute_and_display(mgr, query);
}

/**
 * Clear queue history
 */
static void clear_history(hl7sql_queue_manager_t *mgr) {
    print_header("CLEAR QUEUE HISTORY");

    list_queues(mgr);

    char queue_name[256];
    printf("Enter queue name: ");
    if (!read_line(queue_name, sizeof(queue_name))) {
        printf("Invalid input\n");
        return;
    }

    printf("\nAre you sure you want to clear history for '%s'? (yes/no): ", queue_name);
    char confirm[10];
    if (!read_line(confirm, sizeof(confirm)) || strcmp(confirm, "yes") != 0) {
        printf("Cancelled\n");
        return;
    }

    int result = hl7sql_queue_manager_clear_history(mgr, queue_name);
    if (result == 0) {
        printf("[SUCCESS] History cleared for queue '%s'\n", queue_name);
    } else {
        printf("[ERROR] Failed to clear history for queue '%s'\n", queue_name);
    }
}

/**
 * Execute custom SQL query
 */
static void custom_query(hl7sql_queue_manager_t *mgr) {
    print_header("EXECUTE CUSTOM HL7SQL QUERY");

    printf("Enter HL7SQL query:\n");
    printf("Examples:\n");
    printf("  CREATE QUEUE myqueue\n");
    printf("  SELECT * FROM myqueue WHERE MSH-9 = 'ADT^A01'\n");
    printf("  INSERT INTO myqueue VALUES 'MSH|^~\\&|...|\\rPID|...'\n");
    printf("  POP MESSAGE FROM myqueue\n");
    printf("\nQuery: ");

    char query[4096];
    if (!read_line(query, sizeof(query))) {
        printf("Invalid input\n");
        return;
    }

    execute_and_display(mgr, query);
}

/**
 * Display statistics
 */
static void show_statistics(hl7sql_queue_manager_t *mgr) {
    print_header("QUEUE STATISTICS");

    size_t num_queues = 0;
    char **queue_names = hl7sql_queue_manager_get_all_queues(mgr, &num_queues);

    if (!queue_names || num_queues == 0) {
        printf("No queues exist.\n");
        return;
    }

    size_t total_messages = 0;
    size_t total_history = 0;

    printf("Total Queues: %zu\n\n", num_queues);
    print_separator();

    for (size_t i = 0; i < num_queues; i++) {
        const char *name = queue_names[i];

        hl7sql_store_t *store = hl7sql_queue_manager_get_queue(mgr, name);
        size_t msg_count = store ? hl7sql_store_size(store) : 0;
        size_t history_count = hl7sql_queue_manager_get_history_size(mgr, name);

        total_messages += msg_count;
        total_history += history_count;

        printf("Queue: %s\n", name);
        printf("  Active Messages:    %5zu\n", msg_count);
        printf("  History Messages:   %5zu\n", history_count);
        printf("  Total:              %5zu\n", msg_count + history_count);
        print_separator();
    }

    printf("\nOVERALL STATISTICS\n");
    printf("  Total Active Messages:  %zu\n", total_messages);
    printf("  Total History Messages: %zu\n", total_history);
    printf("  Grand Total:            %zu\n", total_messages + total_history);

    hl7sql_queue_manager_free_queue_names(queue_names, num_queues);
}

/**
 * Main menu
 */
static void show_menu(void) {
    print_header("HL7DB QUEUE TESTER - MAIN MENU");
    printf("\nQUEUE MANAGEMENT:\n");
    printf("  1.  List all queues\n");
    printf("  2.  Create queue\n");
    printf("  3.  Drop queue\n");
    printf("\nVIEWING & QUERYING:\n");
    printf("  4.  View queue messages\n");
    printf("  5.  View queue history\n");
    printf("  6.  Query messages (with WHERE)\n");
    printf("\nINSERTING:\n");
    printf("  7.  Insert sample message\n");
    printf("  8.  Insert custom message\n");
    printf("\nOPERATIONS:\n");
    printf("  9.  Pop message from queue\n");
    printf("  10. Clear queue history\n");
    printf("\nADVANCED:\n");
    printf("  11. Execute custom HL7SQL query\n");
    printf("  12. Show statistics\n");
    printf("\nOTHER:\n");
    printf("  0.  Exit\n");
    print_separator();
    printf("Enter choice: ");
}

/* ====================================================================
 * Main
 * ==================================================================== */

int main(void) {
    /* Initialize logger */
    logger_config_t config = {
        .min_level = LOG_INFO,
        .file = NULL,
        .use_color = 1,
        .include_timestamp = 1,
        .include_level = 1,
        .include_location = 0
    };
    logger_init(&config);

    print_header("HL7DB QUEUE TESTER APPLICATION");
    printf("\nWelcome to the interactive queue tester!\n");
    printf("This tool allows you to create, view, query, and manage HL7 message queues.\n");

    /* Create queue manager */
    printf("\nInitializing queue manager...\n");
    hl7sql_queue_manager_t *mgr = hl7sql_queue_manager_create();
    if (!mgr) {
        fprintf(stderr, "ERROR: Failed to create queue manager\n");
        return 1;
    }
    printf("Queue manager initialized successfully!\n");

    /* Enable persistence */
    persistence_config_t persistence_config = {
        .data_dir = "./data",
        .sync_on_write = 0  /* Async writes for performance */
    };

    if (hl7sql_queue_manager_set_persistence(mgr, &persistence_config) == 0) {
        printf("Persistence enabled (data directory: %s)\n", persistence_config.data_dir);

        /* Load registered queues from registry */
        int loaded = hl7sql_queue_manager_load_registry(mgr);
        if (loaded > 0) {
            printf("Restored %d queue(s) from previous sessions\n", loaded);
        }
    } else {
        printf("WARNING: Failed to enable persistence, running in memory-only mode\n");
    }
    printf("\n");

    /* Main loop */
    char choice[10];
    bool running = true;

    while (running) {
        show_menu();

        if (!read_line(choice, sizeof(choice))) {
            continue;
        }

        int option = atoi(choice);

        switch (option) {
            case 1:
                list_queues(mgr);
                wait_for_enter();
                break;

            case 2:
                create_queue(mgr);
                wait_for_enter();
                break;

            case 3:
                drop_queue(mgr);
                wait_for_enter();
                break;

            case 4:
                view_queue(mgr);
                wait_for_enter();
                break;

            case 5:
                view_history(mgr);
                wait_for_enter();
                break;

            case 6:
                query_messages(mgr);
                wait_for_enter();
                break;

            case 7:
                insert_sample_message(mgr);
                wait_for_enter();
                break;

            case 8:
                insert_custom_message(mgr);
                wait_for_enter();
                break;

            case 9:
                pop_message(mgr);
                wait_for_enter();
                break;

            case 10:
                clear_history(mgr);
                wait_for_enter();
                break;

            case 11:
                custom_query(mgr);
                wait_for_enter();
                break;

            case 12:
                show_statistics(mgr);
                wait_for_enter();
                break;

            case 0:
                running = false;
                print_header("EXITING");
                printf("Cleaning up and shutting down...\n");
                break;

            default:
                printf("\nInvalid choice. Please try again.\n");
                wait_for_enter();
                break;
        }
    }

    /* Cleanup */
    hl7sql_queue_manager_destroy(mgr);
    logger_cleanup();

    printf("\nThank you for using HL7DB Queue Tester!\n");
    printf("Goodbye!\n\n");

    return 0;
}
