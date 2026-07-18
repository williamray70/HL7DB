/**
 * hl7sql_result.h - Query Result Sets
 *
 * Defines structures for representing query results with rows and columns.
 */

#ifndef HL7DB_HL7SQL_RESULT_H
#define HL7DB_HL7SQL_RESULT_H

#include <stddef.h>

/**
 * Query Result Set
 *
 * Represents the result of a query execution with rows and columns.
 */
typedef struct {
    /* Column metadata */
    char **column_names;        /* Array of column names */
    size_t num_columns;         /* Number of columns */

    /* Row data */
    char ***rows;               /* 2D array: rows[row_idx][col_idx] */
    size_t num_rows;            /* Number of rows */
    size_t capacity;            /* Allocated capacity for rows */

    /* Status */
    int error_code;             /* 0 = success, non-zero = error */
    char *error_message;        /* Error description if any */

    /* Statistics */
    size_t rows_affected;       /* For INSERT/UPDATE/DELETE */
} hl7sql_result_t;

/**
 * Create a new result set
 *
 * @param num_columns Number of columns
 * @param column_names Array of column names (will be copied)
 * @return New result set, or NULL on failure
 */
hl7sql_result_t *hl7sql_result_create(size_t num_columns,
                                       const char **column_names);

/**
 * Create an error result
 *
 * @param error_code Error code
 * @param error_message Error message
 * @return Result set with error info
 */
hl7sql_result_t *hl7sql_result_create_error(int error_code,
                                              const char *error_message);

/**
 * Add a row to the result set
 *
 * @param result Result set
 * @param row_values Array of column values (will be copied)
 * @return 0 on success, -1 on failure
 */
int hl7sql_result_add_row(hl7sql_result_t *result, const char **row_values);

/**
 * Get value at specific row and column
 *
 * @param result Result set
 * @param row_idx Row index (0-based)
 * @param col_idx Column index (0-based)
 * @return Value string, or NULL if out of bounds
 */
const char *hl7sql_result_get_value(const hl7sql_result_t *result,
                                      size_t row_idx,
                                      size_t col_idx);

/**
 * Check if result has an error
 *
 * @param result Result set
 * @return 1 if error, 0 if success
 */
int hl7sql_result_has_error(const hl7sql_result_t *result);

/**
 * Print result set (for debugging/display)
 *
 * @param result Result set
 */
void hl7sql_result_print(const hl7sql_result_t *result);

/**
 * Print result set with PHI redaction (HIPAA-compliant)
 *
 * Safe for use in logs, console output, and non-secure contexts.
 * Redacts Protected Health Information from HL7 message data.
 *
 * @param result Result set
 * @param config PHI redaction configuration (NULL = use default)
 */
void hl7sql_result_print_redacted(const hl7sql_result_t *result,
                                   const void *config);

/**
 * Convert result set to string representation
 *
 * @param result Result set
 * @return String representation (caller must free), or NULL on error
 */
char *hl7sql_result_to_string(const hl7sql_result_t *result);

/**
 * Destroy result set and free all resources
 *
 * @param result Result set to destroy
 */
void hl7sql_result_destroy(hl7sql_result_t *result);

#endif /* HL7DB_HL7SQL_RESULT_H */
