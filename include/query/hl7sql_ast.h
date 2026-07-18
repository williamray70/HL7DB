/**
 * hl7sql_ast.h - Abstract Syntax Tree for HL7SQL
 *
 * Defines AST node structures for representing parsed HL7SQL queries.
 */

#ifndef HL7DB_HL7SQL_AST_H
#define HL7DB_HL7SQL_AST_H

#include <stddef.h>

/* Forward declaration */
typedef struct ast_node ast_node_t;

/**
 * AST Node Types
 */
typedef enum {
    /* Statements */
    AST_SELECT_STMT,
    AST_INSERT_STMT,
    AST_CREATE_QUEUE_STMT,    /* CREATE QUEUE queue_name */
    AST_DROP_QUEUE_STMT,      /* DROP QUEUE queue_name */
    AST_POP_MESSAGE_STMT,       /* POP MESSAGE FROM queue_name */
    AST_CLEAR_HISTORY_STMT,     /* CLEAR HISTORY queue_name */

    /* Expressions */
    AST_FIELD_REF,          /* segment.PID[3] */
    AST_LITERAL,            /* '123', 'text', 456 */
    AST_BINARY_OP,          /* =, !=, <, >, AND, OR, LIKE */
    AST_COLUMN,             /* Column in SELECT list */
    AST_COLUMN_LIST,        /* List of columns */
    AST_WILDCARD,           /* * in SELECT * */
    AST_QUEUE_REF,        /* Queue reference (name, *, or ALL) */

    /* Time-based queries */
    AST_TIMESTAMP_REF,      /* timestamp field reference */
    AST_TIME_FUNCTION,      /* LAST_N_HOURS(), LAST_N_DAYS() */
    AST_BETWEEN_OP,         /* BETWEEN operator */

    /* Aggregation */
    AST_COUNT,              /* COUNT(*) or COUNT(expr) */
    AST_GROUP_BY,           /* GROUP BY clause */
} ast_node_type_t;

/**
 * Binary operators
 */
typedef enum {
    OP_EQ,          /* = */
    OP_NE,          /* !=  or <> */
    OP_LT,          /* < */
    OP_LE,          /* <= */
    OP_GT,          /* > */
    OP_GE,          /* >= */
    OP_LIKE,        /* LIKE */
    OP_AND,         /* AND */
    OP_OR,          /* OR */
} binary_op_type_t;

/**
 * Literal types
 */
typedef enum {
    LITERAL_STRING,
    LITERAL_NUMBER,
} literal_type_t;

/**
 * Queue reference types
 */
typedef enum {
    QUEUE_NAMED,      /* Specific queue name */
    QUEUE_HISTORY,    /* queue_name.history */
    QUEUE_WILDCARD,   /* * - all queues */
    QUEUE_ALL,        /* ALL keyword */
} queue_ref_type_t;

/**
 * Time function types
 */
typedef enum {
    TIME_FUNC_LAST_N_HOURS,    /* LAST_N_HOURS(n) */
    TIME_FUNC_LAST_N_DAYS,     /* LAST_N_DAYS(n) */
} time_function_type_t;

/**
 * Field Reference
 * Represents: segment.SEGMENT_ID[field_num]
 * Example: segment.PID[3]
 */
typedef struct {
    char *segment_id;       /* e.g., "PID", "MSH" */
    int field_num;          /* 1-based field number */
} ast_field_ref_t;

/**
 * Literal Value
 */
typedef struct {
    literal_type_t type;
    union {
        char *string_value;
        long long int_value;
    };
} ast_literal_t;

/**
 * Binary Operation
 */
typedef struct {
    binary_op_type_t op;
    ast_node_t *left;
    ast_node_t *right;
} ast_binary_op_t;

/**
 * Column (for SELECT list)
 */
typedef struct {
    ast_node_t *expr;       /* Expression (field ref, aggregate, etc.) */
    char *alias;            /* AS alias (optional) */
} ast_column_t;

/**
 * Column List
 */
typedef struct {
    ast_node_t **columns;   /* Array of AST_COLUMN nodes */
    size_t num_columns;
} ast_column_list_t;

/**
 * Queue Reference
 * Represents: queue_name | queue_name.history | * | ALL
 */
typedef struct {
    queue_ref_type_t type;
    char *name;         /* NULL for wildcards, queue name for NAMED/HISTORY */
} ast_queue_ref_t;

/**
 * SELECT Statement
 */
typedef struct {
    ast_node_t *columns;    /* AST_COLUMN_LIST or AST_WILDCARD */
    ast_node_t *queue;    /* AST_QUEUE_REF - target queue(s) */
    ast_node_t *where;      /* WHERE clause (optional, can be NULL) */
    ast_node_t *group_by;   /* GROUP BY clause (optional, can be NULL) */
} ast_select_stmt_t;

/**
 * INSERT Statement
 */
typedef struct {
    char *queue_name;     /* Target queue name */
    char *message_data;     /* Raw HL7 message string */
} ast_insert_stmt_t;

/**
 * CREATE QUEUE Statement
 */
typedef struct {
    char *queue_name;
} ast_create_queue_stmt_t;

/**
 * DROP QUEUE Statement
 */
typedef struct {
    char *queue_name;
} ast_drop_queue_stmt_t;

/**
 * POP MESSAGE Statement
 */
typedef struct {
    char *queue_name;
} ast_pop_message_stmt_t;

/**
 * CLEAR HISTORY Statement
 */
typedef struct {
    char *queue_name;
} ast_clear_history_stmt_t;

/**
 * COUNT Aggregation
 */
typedef struct {
    ast_node_t *expr;       /* Expression to count (NULL = COUNT(*)) */
} ast_count_t;

/**
 * GROUP BY Clause
 */
typedef struct {
    ast_node_t **fields;    /* Array of field references */
    size_t num_fields;
} ast_group_by_t;

/**
 * Timestamp Reference
 * Represents: timestamp keyword
 */
typedef struct {
    /* No fields needed - simple marker */
    int _unused;
} ast_timestamp_ref_t;

/**
 * Time Function
 * Represents: LAST_N_HOURS(n) or LAST_N_DAYS(n)
 */
typedef struct {
    time_function_type_t func_type;
    int n_value;    /* Number of hours/days */
} ast_time_function_t;

/**
 * BETWEEN Operation
 * Represents: expr BETWEEN lower AND upper
 */
typedef struct {
    ast_node_t *expr;           /* Expression to test */
    ast_node_t *lower_bound;    /* Lower bound (inclusive) */
    ast_node_t *upper_bound;    /* Upper bound (inclusive) */
} ast_between_op_t;

/**
 * AST Node (Union of all node types)
 */
struct ast_node {
    ast_node_type_t type;

    union {
        ast_select_stmt_t select_stmt;
        ast_insert_stmt_t insert_stmt;
        ast_create_queue_stmt_t create_queue_stmt;
        ast_drop_queue_stmt_t drop_queue_stmt;
        ast_pop_message_stmt_t pop_message_stmt;
        ast_clear_history_stmt_t clear_history_stmt;
        ast_queue_ref_t queue_ref;
        ast_field_ref_t field_ref;
        ast_literal_t literal;
        ast_binary_op_t binary_op;
        ast_column_t column;
        ast_column_list_t column_list;
        ast_count_t count;
        ast_group_by_t group_by;
        ast_timestamp_ref_t timestamp_ref;
        ast_time_function_t time_function;
        ast_between_op_t between_op;
    };
};

/**
 * Create AST nodes
 */
ast_node_t *ast_create_select(ast_node_t *columns,
                                ast_node_t *queue,
                                ast_node_t *where,
                                ast_node_t *group_by);

ast_node_t *ast_create_insert(const char *queue_name,
                                const char *message_data);

ast_node_t *ast_create_create_queue(const char *queue_name);

ast_node_t *ast_create_drop_queue(const char *queue_name);

ast_node_t *ast_create_pop_message(const char *queue_name);

ast_node_t *ast_create_clear_history(const char *queue_name);

ast_node_t *ast_create_queue_ref_named(const char *name);

ast_node_t *ast_create_queue_ref_history(const char *name);

ast_node_t *ast_create_queue_ref_wildcard(void);

ast_node_t *ast_create_queue_ref_all(void);

ast_node_t *ast_create_field_ref(const char *segment_id, int field_num);

ast_node_t *ast_create_literal_string(const char *value);

ast_node_t *ast_create_literal_number(long long value);

ast_node_t *ast_create_binary_op(binary_op_type_t op,
                                   ast_node_t *left,
                                   ast_node_t *right);

ast_node_t *ast_create_column(ast_node_t *expr, const char *alias);

ast_node_t *ast_create_column_list(ast_node_t **columns, size_t num_columns);

ast_node_t *ast_create_wildcard(void);

ast_node_t *ast_create_count(ast_node_t *expr);

ast_node_t *ast_create_group_by(ast_node_t **fields, size_t num_fields);

ast_node_t *ast_create_timestamp_ref(void);

ast_node_t *ast_create_time_function(time_function_type_t func_type, int n_value);

ast_node_t *ast_create_between(ast_node_t *expr,
                                 ast_node_t *lower_bound,
                                 ast_node_t *upper_bound);

/**
 * Destroy AST (recursively free all nodes)
 */
void ast_destroy(ast_node_t *node);

/**
 * Print AST for debugging
 */
void ast_print(const ast_node_t *node, int indent);

/**
 * Get operator string
 */
const char *ast_get_op_string(binary_op_type_t op);

#endif /* HL7DB_HL7SQL_AST_H */
