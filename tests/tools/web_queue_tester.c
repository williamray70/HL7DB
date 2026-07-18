/**
 * web_queue_tester.c - Modern Web Interface for HL7 Queue Tester
 *
 * HTTP server exposing queue management functionality via REST API
 * Serves a modern web interface for interacting with HL7 message queues
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "query/hl7sql_queue_manager.h"
#include "query/hl7sql_executor.h"
#include "query/hl7sql_parser.h"
#include "query/hl7sql_result.h"
#include "hl7/hl7_message.h"
#include "hl7/hl7_parser.h"
#include "util/logger.h"

/* Global queue manager */
static hl7sql_queue_manager_t *g_queue_mgr = NULL;
static pthread_mutex_t g_mgr_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Sample HL7 Messages */
static const char *SAMPLE_ADT_A01 =
    "MSH|^~\\&|SENDING_APP|SENDING_FAC|RECV_APP|RECV_FAC|20260618120000||ADT^A01|MSG00001|P|2.5\r"
    "EVN|A01|20260618120000\r"
    "PID|1||12345^^^MRN||DOE^JOHN^A||19800101|M||W|123 MAIN ST^^ANYTOWN^CA^12345||555-1234|||M\r"
    "PV1|1|I|ICU^101^1|||123^SMITH^JOHN|||MED||||1|||123^SMITH^JOHN|INP|12345|||||||||||||||||||||||||20260618120000\r";

static const char *SAMPLE_ORU_R01 =
    "MSH|^~\\&|LAB_SYS|LAB_FAC|RECV_APP|RECV_FAC|20260618130000||ORU^R01|MSG00002|P|2.5\r"
    "PID|1||67890^^^MRN||SMITH^JANE^B||19900215|F||B|456 ELM ST^^OTHERTOWN^NY^67890||555-5678|||F\r"
    "OBR|1|ORD12345|LAB54321|CBC^COMPLETE BLOOD COUNT|||20260618120000\r"
    "OBX|1|NM|WBC^White Blood Count||7.5|10*3/uL|4.5-11.0|N|||F\r"
    "OBX|2|NM|RBC^Red Blood Count||4.8|10*6/uL|4.2-5.9|N|||F\r";

static const char *SAMPLE_ORM_O01 =
    "MSH|^~\\&|ORDER_APP|ORDER_FAC|RECV_APP|RECV_FAC|20260618140000||ORM^O01|MSG00003|P|2.5\r"
    "PID|1||11111^^^MRN||JONES^ROBERT^C||19750520|M||W|789 OAK AVE^^SOMECITY^TX^78901||555-9012|||M\r"
    "ORC|NW|ORD99999||||||20260618140000\r"
    "OBR|1|ORD99999||XRAY^CHEST X-RAY|||20260618140000\r";

/* ====================================================================
 * HTTP Utilities
 * ==================================================================== */

/**
 * URL decode a string in-place
 */
static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

/**
 * Escape JSON string
 */
static char* json_escape(const char *str) {
    if (!str) return strdup("null");

    size_t len = strlen(str);
    char *escaped = malloc(len * 6 + 3); // Worst case: all chars need escaping + quotes + null
    if (!escaped) return NULL;

    char *p = escaped;
    *p++ = '"';

    for (size_t i = 0; i < len; i++) {
        unsigned char c = str[i];
        switch (c) {
            case '"':  *p++ = '\\'; *p++ = '"'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '\b': *p++ = '\\'; *p++ = 'b'; break;
            case '\f': *p++ = '\\'; *p++ = 'f'; break;
            case '\n': *p++ = '\\'; *p++ = 'n'; break;
            case '\r': *p++ = '\\'; *p++ = 'r'; break;
            case '\t': *p++ = '\\'; *p++ = 't'; break;
            default:
                if (c < 32) {
                    p += sprintf(p, "\\u%04x", c);
                } else {
                    *p++ = c;
                }
        }
    }

    *p++ = '"';
    *p = '\0';

    return escaped;
}

/**
 * Send HTTP response
 */
static void send_response(int client_fd, int status_code, const char *status_text,
                         const char *content_type, const char *body) {
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text, content_type, body ? strlen(body) : 0);

    ssize_t result = write(client_fd, header, header_len);
    if (result < 0) {
        LOG_WARN("Failed to write HTTP header: %s", strerror(errno));
        return;
    }

    if (body) {
        result = write(client_fd, body, strlen(body));
        if (result < 0) {
            LOG_WARN("Failed to write HTTP body: %s", strerror(errno));
        }
    }
}

/**
 * Send JSON response
 */
static void send_json(int client_fd, const char *json) {
    send_response(client_fd, 200, "OK", "application/json", json);
}

/**
 * Send error JSON
 */
static void send_error(int client_fd, const char *message) {
    char *escaped = json_escape(message);
    char json[1024];
    snprintf(json, sizeof(json), "{\"error\":%s}", escaped);
    free(escaped);
    send_response(client_fd, 400, "Bad Request", "application/json", json);
}

/* ====================================================================
 * API Handlers
 * ==================================================================== */

/**
 * GET /api/queues - List all queues
 */
static void handle_list_queues(int client_fd) {
    pthread_mutex_lock(&g_mgr_mutex);

    size_t num_queues = 0;
    char **queue_names = hl7sql_queue_manager_get_all_queues(g_queue_mgr, &num_queues);

    char *response = malloc(65536);
    if (!response) {
        pthread_mutex_unlock(&g_mgr_mutex);
        send_error(client_fd, "Out of memory");
        return;
    }

    strcpy(response, "{\"queues\":[");

    for (size_t i = 0; i < num_queues; i++) {
        const char *name = queue_names[i];
        hl7sql_store_t *store = hl7sql_queue_manager_get_queue(g_queue_mgr, name);
        size_t msg_count = store ? hl7sql_store_size(store) : 0;
        size_t history_count = hl7sql_queue_manager_get_history_size(g_queue_mgr, name);

        char *escaped_name = json_escape(name);
        char item[512];
        snprintf(item, sizeof(item), "%s{\"name\":%s,\"messages\":%zu,\"history\":%zu}",
                 i > 0 ? "," : "", escaped_name, msg_count, history_count);
        free(escaped_name);
        strcat(response, item);
    }

    strcat(response, "]}");

    hl7sql_queue_manager_free_queue_names(queue_names, num_queues);
    pthread_mutex_unlock(&g_mgr_mutex);

    send_json(client_fd, response);
    free(response);
}

/**
 * POST /api/query - Execute HL7SQL query
 */
static void handle_query(int client_fd, const char *query_string) {
    if (!query_string) {
        send_error(client_fd, "Missing query parameter");
        return;
    }

    char decoded_query[4096];
    url_decode(decoded_query, query_string);

    pthread_mutex_lock(&g_mgr_mutex);
    hl7sql_result_t *result = hl7sql_query(g_queue_mgr, decoded_query);
    pthread_mutex_unlock(&g_mgr_mutex);

    if (!result) {
        send_error(client_fd, "Query execution failed");
        return;
    }

    char *response = malloc(1048576); // 1MB buffer
    if (!response) {
        hl7sql_result_destroy(result);
        send_error(client_fd, "Out of memory");
        return;
    }

    if (result->error_code != 0) {
        char *escaped_error = json_escape(result->error_message ? result->error_message : "Unknown error");
        snprintf(response, 1048576, "{\"success\":false,\"error\":%s}", escaped_error);
        free(escaped_error);
    } else {
        strcpy(response, "{\"success\":true,");
        char temp[256];
        snprintf(temp, sizeof(temp), "\"rows_affected\":%zu,\"num_rows\":%zu,\"rows\":[",
                 result->rows_affected, result->num_rows);
        strcat(response, temp);

        for (size_t i = 0; i < result->num_rows; i++) {
            if (i > 0) strcat(response, ",");
            strcat(response, "{");

            for (size_t j = 0; j < result->num_columns; j++) {
                if (j > 0) strcat(response, ",");

                char *col_name = json_escape(result->column_names[j]);
                char *col_value = json_escape(result->rows[i][j]);

                strcat(response, col_name);
                strcat(response, ":");
                strcat(response, col_value);

                free(col_name);
                free(col_value);
            }

            strcat(response, "}");
        }

        strcat(response, "]}");
    }

    hl7sql_result_destroy(result);
    send_json(client_fd, response);
    free(response);
}

/**
 * POST /api/insert - Insert HL7 message directly (bypass SQL parsing)
 */
static void handle_insert_message(int client_fd, const char *request) {
    // Find JSON body
    char *body = strstr(request, "\r\n\r\n");
    if (!body) {
        send_error(client_fd, "Missing request body");
        return;
    }
    body += 4; // Skip \r\n\r\n

    // Parse JSON: {"queue":"name","message":"..."}
    // Simple parser - find queue name
    char *queue_start = strstr(body, "\"queue\":\"");
    if (!queue_start) {
        send_error(client_fd, "Missing queue parameter");
        return;
    }
    queue_start += 9; // Skip "queue":"

    char *queue_end = strchr(queue_start, '"');
    if (!queue_end) {
        send_error(client_fd, "Invalid queue parameter");
        return;
    }

    char queue_name[256];
    size_t queue_len = queue_end - queue_start;
    if (queue_len >= sizeof(queue_name)) {
        send_error(client_fd, "Queue name too long");
        return;
    }
    memcpy(queue_name, queue_start, queue_len);
    queue_name[queue_len] = '\0';

    // Find message data
    char *msg_start = strstr(body, "\"message\":\"");
    if (!msg_start) {
        send_error(client_fd, "Missing message parameter");
        return;
    }
    msg_start += 11; // Skip "message":"

    // Find end of message (watch for escaped quotes and JSON control chars)
    char *msg_end = msg_start;
    while (*msg_end) {
        if (*msg_end == '"' && (msg_end == msg_start || *(msg_end-1) != '\\')) {
            break;
        }
        msg_end++;
    }

    // Unescape JSON string (handle \r, \n, \\, \", etc.)
    size_t max_len = msg_end - msg_start;
    char *message = malloc(max_len + 1);
    if (!message) {
        send_error(client_fd, "Out of memory");
        return;
    }

    char *src = msg_start;
    char *dst = message;
    while (src < msg_end) {
        if (*src == '\\' && src + 1 < msg_end) {
            // Handle escape sequences
            src++; // Skip backslash
            switch (*src) {
                case 'r': *dst++ = '\r'; break;
                case 'n': *dst++ = '\n'; break;
                case 't': *dst++ = '\t'; break;
                case '\\': *dst++ = '\\'; break;
                case '"': *dst++ = '"'; break;
                case '/': *dst++ = '/'; break;
                default:
                    // Unknown escape - keep both characters
                    *dst++ = '\\';
                    *dst++ = *src;
                    break;
            }
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';

    // Parse HL7 message
    hl7_message_t *hl7_msg = hl7_parse(message, NULL);
    if (!hl7_msg) {
        free(message);
        send_error(client_fd, "Invalid HL7 message");
        return;
    }

    // Get queue and insert message
    pthread_mutex_lock(&g_mgr_mutex);

    hl7sql_store_t *store = hl7sql_queue_manager_get_queue(g_queue_mgr, queue_name);
    if (!store) {
        pthread_mutex_unlock(&g_mgr_mutex);
        hl7_message_destroy(hl7_msg);
        free(message);
        send_error(client_fd, "Queue not found");
        return;
    }

    int result = hl7sql_store_insert(store, hl7_msg);

    // Persist to disk if enabled
    channel_persistence_t *ph = hashmap_get(g_queue_mgr->persistence_handles, queue_name);
    if (ph) {
        if (persistence_append_message(ph, hl7_msg->msg_id, hl7_msg) != 0) {
            LOG_WARN("Failed to persist message %llu for '%s'",
                     (unsigned long long)hl7_msg->msg_id, queue_name);
        }
    }

    pthread_mutex_unlock(&g_mgr_mutex);

    free(message);

    if (result != 0) {
        send_error(client_fd, "Failed to insert message");
        return;
    }

    // Success response
    char response[512];
    snprintf(response, sizeof(response),
             "{\"success\":true,\"message\":\"Message inserted successfully\"}");
    send_json(client_fd, response);
}

/**
 * GET /api/samples - Get sample messages
 */
static void handle_samples(int client_fd) {
    char *response = malloc(16384);
    if (!response) {
        send_error(client_fd, "Out of memory");
        return;
    }

    char *adt = json_escape(SAMPLE_ADT_A01);
    char *oru = json_escape(SAMPLE_ORU_R01);
    char *orm = json_escape(SAMPLE_ORM_O01);

    snprintf(response, 16384,
             "{\"samples\":["
             "{\"name\":\"ADT^A01 (Patient Admission)\",\"message\":%s},"
             "{\"name\":\"ORU^R01 (Lab Result)\",\"message\":%s},"
             "{\"name\":\"ORM^O01 (Order)\",\"message\":%s}"
             "]}",
             adt, oru, orm);

    free(adt);
    free(oru);
    free(orm);

    send_json(client_fd, response);
    free(response);
}

/**
 * GET /api/statistics - Get statistics
 */
static void handle_statistics(int client_fd) {
    pthread_mutex_lock(&g_mgr_mutex);

    size_t num_queues = 0;
    char **queue_names = hl7sql_queue_manager_get_all_queues(g_queue_mgr, &num_queues);

    size_t total_messages = 0;
    size_t total_history = 0;

    for (size_t i = 0; i < num_queues; i++) {
        const char *name = queue_names[i];
        hl7sql_store_t *store = hl7sql_queue_manager_get_queue(g_queue_mgr, name);
        size_t msg_count = store ? hl7sql_store_size(store) : 0;
        size_t history_count = hl7sql_queue_manager_get_history_size(g_queue_mgr, name);

        total_messages += msg_count;
        total_history += history_count;
    }

    hl7sql_queue_manager_free_queue_names(queue_names, num_queues);
    pthread_mutex_unlock(&g_mgr_mutex);

    char response[1024];
    snprintf(response, sizeof(response),
             "{\"total_queues\":%zu,\"total_messages\":%zu,\"total_history\":%zu,\"grand_total\":%zu}",
             num_queues, total_messages, total_history, total_messages + total_history);

    send_json(client_fd, response);
}

/* ====================================================================
 * HTTP Server
 * ==================================================================== */

/**
 * Parse HTTP request and route to handler
 */
static void handle_client(int client_fd) {
    char buffer[65536];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';

    // Parse request line
    char method[16], path[1024], protocol[16];
    if (sscanf(buffer, "%15s %1023s %15s", method, path, protocol) != 3) {
        send_response(client_fd, 400, "Bad Request", "text/plain", "Invalid request");
        close(client_fd);
        return;
    }

    // Extract query string if present
    char *query_start = strchr(path, '?');
    char *query_string = NULL;
    if (query_start) {
        *query_start = '\0';
        query_string = query_start + 1;
    }

    // Route requests
    if (strcmp(path, "/") == 0) {
        // Serve index.html
        extern const char _binary_web_index_html_start[];
        extern const char _binary_web_index_html_end[];
        size_t html_size = _binary_web_index_html_end - _binary_web_index_html_start;

        char header[512];
        int header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",
            html_size);

        ssize_t result = write(client_fd, header, header_len);
        (void)result; // Ignore errors - client may have disconnected
        result = write(client_fd, _binary_web_index_html_start, html_size);
        (void)result;
    }
    else if (strcmp(path, "/api/queues") == 0) {
        handle_list_queues(client_fd);
    }
    else if (strcmp(path, "/api/query") == 0) {
        // Extract query from POST body or GET parameter
        if (strcmp(method, "POST") == 0) {
            char *body = strstr(buffer, "\r\n\r\n");
            if (body) {
                body += 4;
                char *query_param = strstr(body, "query=");
                if (query_param) {
                    handle_query(client_fd, query_param + 6);
                } else {
                    send_error(client_fd, "Missing query parameter");
                }
            } else {
                send_error(client_fd, "Missing request body");
            }
        } else if (query_string) {
            char *query_param = strstr(query_string, "query=");
            if (query_param) {
                handle_query(client_fd, query_param + 6);
            } else {
                send_error(client_fd, "Missing query parameter");
            }
        } else {
            send_error(client_fd, "Missing query parameter");
        }
    }
    else if (strcmp(path, "/api/insert") == 0 && strcmp(method, "POST") == 0) {
        handle_insert_message(client_fd, buffer);
    }
    else if (strcmp(path, "/api/samples") == 0) {
        handle_samples(client_fd);
    }
    else if (strcmp(path, "/api/statistics") == 0) {
        handle_statistics(client_fd);
    }
    else if (strcmp(path, "/api/health") == 0) {
        char health[256];
        snprintf(health, sizeof(health),
                 "{\"status\":\"healthy\",\"version\":\"1.0.0\",\"features\":[\"persistence\",\"indexing\"]}");
        send_json(client_fd, health);
    }
    else {
        send_response(client_fd, 404, "Not Found", "text/plain", "Not Found");
    }

    close(client_fd);
}

/**
 * Main server loop
 */
static void* server_thread(void *arg) {
    int server_fd = *(int*)arg;

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }

        handle_client(client_fd);
    }

    return NULL;
}

/* ====================================================================
 * Main
 * ==================================================================== */

int main(int argc, char *argv[]) {
    int port = 8080;

    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", argv[1]);
            return 1;
        }
    }

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

    printf("================================================================================\n");
    printf("  HL7DB QUEUE TESTER - WEB INTERFACE\n");
    printf("================================================================================\n\n");

    /* Create queue manager */
    printf("Initializing queue manager...\n");
    g_queue_mgr = hl7sql_queue_manager_create();
    if (!g_queue_mgr) {
        fprintf(stderr, "ERROR: Failed to create queue manager\n");
        return 1;
    }
    printf("Queue manager initialized successfully!\n");

    /* Enable persistence */
    persistence_config_t persistence_config = {
        .data_dir = "./data",
        .sync_on_write = 0  /* Async writes for performance */
    };

    if (hl7sql_queue_manager_set_persistence(g_queue_mgr, &persistence_config) == 0) {
        printf("Persistence enabled (data directory: %s)\n", persistence_config.data_dir);

        /* Load registered queues from registry */
        int loaded = hl7sql_queue_manager_load_registry(g_queue_mgr);
        if (loaded > 0) {
            printf("Restored %d queue(s) from previous sessions\n", loaded);
        }
    } else {
        printf("WARNING: Failed to enable persistence, running in memory-only mode\n");
    }
    printf("\n");

    /* Create server socket */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return 1;
    }

    printf("Web server started successfully!\n");
    printf("Open your browser to: http://localhost:%d\n\n", port);
    printf("Press Ctrl+C to stop the server\n");
    printf("================================================================================\n\n");

    /* Start server thread */
    pthread_t thread;
    pthread_create(&thread, NULL, server_thread, &server_fd);
    pthread_join(thread, NULL);

    /* Cleanup */
    close(server_fd);
    hl7sql_queue_manager_destroy(g_queue_mgr);
    logger_cleanup();

    return 0;
}
