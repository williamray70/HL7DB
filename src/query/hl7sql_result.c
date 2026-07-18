/**
 * hl7sql_result.c - Query Result Sets Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include "query/hl7sql_result.h"
#include "util/phi_redaction.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_ROW_CAPACITY 16

/**
 * Create a new result set
 */
hl7sql_result_t *hl7sql_result_create(size_t num_columns,
                                       const char **column_names) {
    if (num_columns == 0) {
        return NULL;
    }

    hl7sql_result_t *result = calloc(1, sizeof(hl7sql_result_t));
    if (!result) {
        return NULL;
    }

    /* Allocate column names */
    result->column_names = calloc(num_columns, sizeof(char *));
    if (!result->column_names) {
        free(result);
        return NULL;
    }

    /* Copy column names */
    for (size_t i = 0; i < num_columns; i++) {
        if (column_names && column_names[i]) {
            result->column_names[i] = strdup(column_names[i]);
            if (!result->column_names[i]) {
                /* Cleanup on failure */
                for (size_t j = 0; j < i; j++) {
                    free(result->column_names[j]);
                }
                free(result->column_names);
                free(result);
                return NULL;
            }
        }
    }

    result->num_columns = num_columns;

    /* Allocate initial row capacity */
    result->rows = calloc(INITIAL_ROW_CAPACITY, sizeof(char **));
    if (!result->rows) {
        for (size_t i = 0; i < num_columns; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
        free(result);
        return NULL;
    }

    result->capacity = INITIAL_ROW_CAPACITY;
    result->num_rows = 0;
    result->error_code = 0;
    result->error_message = NULL;

    return result;
}

/**
 * Create an error result
 */
hl7sql_result_t *hl7sql_result_create_error(int error_code,
                                              const char *error_message) {
    hl7sql_result_t *result = calloc(1, sizeof(hl7sql_result_t));
    if (!result) {
        return NULL;
    }

    result->error_code = error_code;

    if (error_message) {
        result->error_message = strdup(error_message);
    }

    return result;
}

/**
 * Add a row to the result set
 */
int hl7sql_result_add_row(hl7sql_result_t *result, const char **row_values) {
    if (!result || !row_values) {
        return -1;
    }

    /* Expand capacity if needed */
    if (result->num_rows >= result->capacity) {
        size_t new_capacity = result->capacity * 2;
        char ***new_rows = realloc(result->rows, new_capacity * sizeof(char **));
        if (!new_rows) {
            return -1;
        }
        result->rows = new_rows;
        result->capacity = new_capacity;
    }

    /* Allocate row */
    result->rows[result->num_rows] = calloc(result->num_columns, sizeof(char *));
    if (!result->rows[result->num_rows]) {
        return -1;
    }

    /* Copy column values */
    for (size_t i = 0; i < result->num_columns; i++) {
        if (row_values[i]) {
            result->rows[result->num_rows][i] = strdup(row_values[i]);
            if (!result->rows[result->num_rows][i]) {
                /* Cleanup on failure */
                for (size_t j = 0; j < i; j++) {
                    free(result->rows[result->num_rows][j]);
                }
                free(result->rows[result->num_rows]);
                return -1;
            }
        } else {
            result->rows[result->num_rows][i] = NULL;
        }
    }

    result->num_rows++;
    return 0;
}

/**
 * Get value at specific row and column
 */
const char *hl7sql_result_get_value(const hl7sql_result_t *result,
                                      size_t row_idx,
                                      size_t col_idx) {
    if (!result || row_idx >= result->num_rows || col_idx >= result->num_columns) {
        return NULL;
    }

    return result->rows[row_idx][col_idx];
}

/**
 * Check if result has an error
 */
int hl7sql_result_has_error(const hl7sql_result_t *result) {
    return result ? (result->error_code != 0) : 1;
}

/**
 * Print result set
 */
void hl7sql_result_print(const hl7sql_result_t *result) {
    if (!result) {
        printf("NULL result\n");
        return;
    }

    /* Print error if present */
    if (result->error_code != 0) {
        printf("ERROR %d: %s\n", result->error_code,
               result->error_message ? result->error_message : "Unknown error");
        return;
    }

    /* Print column headers */
    if (result->num_columns > 0) {
        for (size_t i = 0; i < result->num_columns; i++) {
            printf("%-20s ", result->column_names[i] ? result->column_names[i] : "");
        }
        printf("\n");

        /* Print separator */
        for (size_t i = 0; i < result->num_columns; i++) {
            printf("-------------------- ");
        }
        printf("\n");
    }

    /* Print rows */
    for (size_t row = 0; row < result->num_rows; row++) {
        for (size_t col = 0; col < result->num_columns; col++) {
            const char *value = result->rows[row][col];
            printf("%-20s ", value ? value : "NULL");
        }
        printf("\n");
    }

    /* Print summary */
    printf("\n%zu row(s) returned\n", result->num_rows);

    if (result->rows_affected > 0) {
        printf("%zu row(s) affected\n", result->rows_affected);
    }
}

/**
 * Print result set with PHI redaction (HIPAA-compliant)
 */
void hl7sql_result_print_redacted(const hl7sql_result_t *result,
                                   const void *config) {
    if (!result) {
        printf("NULL result\n");
        return;
    }

    const phi_redaction_config_t *phi_config = (const phi_redaction_config_t *)config;
    phi_redaction_config_t default_config;

    /* Use default config if none provided */
    if (!phi_config) {
        default_config = phi_redaction_get_default_config();
        phi_config = &default_config;
    }

    /* Print error if present (errors don't contain PHI) */
    if (result->error_code != 0) {
        printf("ERROR %d: %s\n", result->error_code,
               result->error_message ? result->error_message : "Unknown error");
        return;
    }

    /* Print column headers */
    if (result->num_columns > 0) {
        for (size_t i = 0; i < result->num_columns; i++) {
            printf("%-20s ", result->column_names[i] ? result->column_names[i] : "");
        }
        printf("\n");

        /* Print separator */
        for (size_t i = 0; i < result->num_columns; i++) {
            printf("-------------------- ");
        }
        printf("\n");
    }

    /* Print rows with redaction */
    for (size_t row = 0; row < result->num_rows; row++) {
        for (size_t col = 0; col < result->num_columns; col++) {
            const char *value = result->rows[row][col];

            /* Check if this is a raw_message column that needs redaction */
            const char *col_name = (col < result->num_columns && result->column_names[col])
                                     ? result->column_names[col] : "";

            if (value && (strcmp(col_name, "raw_message") == 0 ||
                          strcmp(col_name, "message") == 0)) {
                /* This is an HL7 message - redact it */
                char *redacted = phi_redact_message(value, phi_config);
                if (redacted) {
                    /* Truncate for display */
                    size_t len = strlen(redacted);
                    if (len > 50) {
                        printf("%.47s... ", redacted);
                    } else {
                        printf("%-20s ", redacted);
                    }
                    free(redacted);
                } else {
                    printf("%-20s ", "[ERROR]");
                }
            } else {
                /* Non-message column - display as-is (msg_id, etc.) */
                printf("%-20s ", value ? value : "NULL");
            }
        }
        printf("\n");
    }

    /* Print summary */
    printf("\n%zu row(s) returned (PHI REDACTED)\n", result->num_rows);

    if (result->rows_affected > 0) {
        printf("%zu row(s) affected\n", result->rows_affected);
    }
}

/**
 * Convert result set to string
 */
char *hl7sql_result_to_string(const hl7sql_result_t *result) {
    if (!result) {
        return strdup("NULL result");
    }

    /* Handle error case */
    if (result->error_code != 0) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "ERROR %d: %s",
                 result->error_code,
                 result->error_message ? result->error_message : "Unknown error");
        return strdup(buffer);
    }

    /* For empty results or non-SELECT queries */
    if (result->num_rows == 0) {
        if (result->rows_affected > 0) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "OK (%zu rows affected)",
                     result->rows_affected);
            return strdup(buffer);
        }
        return strdup("OK (0 rows)");
    }

    /* Calculate buffer size needed */
    size_t buffer_size = 1024; /* Initial size */
    for (size_t row = 0; row < result->num_rows; row++) {
        for (size_t col = 0; col < result->num_columns; col++) {
            const char *value = result->rows[row][col];
            if (value) {
                buffer_size += strlen(value) + 3; /* value + delimiter */
            }
        }
        buffer_size += 2; /* newline */
    }

    /* Allocate buffer */
    char *output = malloc(buffer_size);
    if (!output) {
        return NULL;
    }

    size_t offset = 0;

    /* Format as simple text table */
    for (size_t row = 0; row < result->num_rows; row++) {
        for (size_t col = 0; col < result->num_columns; col++) {
            const char *value = result->rows[row][col];
            if (col > 0) {
                offset += snprintf(output + offset, buffer_size - offset, " | ");
            }
            offset += snprintf(output + offset, buffer_size - offset, "%s",
                              value ? value : "NULL");
        }
        offset += snprintf(output + offset, buffer_size - offset, "\n");
    }

    /* Add summary */
    offset += snprintf(output + offset, buffer_size - offset,
                      "\n%zu row(s) returned", result->num_rows);

    return output;
}

/**
 * Destroy result set
 */
void hl7sql_result_destroy(hl7sql_result_t *result) {
    if (!result) {
        return;
    }

    /* Free column names */
    if (result->column_names) {
        for (size_t i = 0; i < result->num_columns; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }

    /* Free rows */
    if (result->rows) {
        for (size_t row = 0; row < result->num_rows; row++) {
            if (result->rows[row]) {
                for (size_t col = 0; col < result->num_columns; col++) {
                    free(result->rows[row][col]);
                }
                free(result->rows[row]);
            }
        }
        free(result->rows);
    }

    /* Free error message */
    free(result->error_message);

    free(result);
}
