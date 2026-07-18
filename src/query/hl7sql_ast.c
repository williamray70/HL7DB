/**
 * hl7sql_ast.c - AST Implementation
 */

#define _POSIX_C_SOURCE 200809L

#include "query/hl7sql_ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Allocate and initialize AST node
 */
static ast_node_t *ast_alloc(ast_node_type_t type) {
    ast_node_t *node = calloc(1, sizeof(ast_node_t));
    if (!node) {
        return NULL;
    }
    node->type = type;
    return node;
}

/**
 * Create SELECT statement
 */
ast_node_t *ast_create_select(ast_node_t *columns,
                                ast_node_t *queue,
                                ast_node_t *where,
                                ast_node_t *group_by) {
    ast_node_t *node = ast_alloc(AST_SELECT_STMT);
    if (!node) {
        return NULL;
    }

    node->select_stmt.columns = columns;
    node->select_stmt.queue = queue;
    node->select_stmt.where = where;
    node->select_stmt.group_by = group_by;

    return node;
}

/**
 * Create INSERT statement
 */
ast_node_t *ast_create_insert(const char *queue_name,
                                const char *message_data) {
    if (!queue_name || !message_data) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_INSERT_STMT);
    if (!node) {
        return NULL;
    }

    node->insert_stmt.queue_name = strdup(queue_name);
    if (!node->insert_stmt.queue_name) {
        free(node);
        return NULL;
    }

    node->insert_stmt.message_data = strdup(message_data);
    if (!node->insert_stmt.message_data) {
        free(node->insert_stmt.queue_name);
        free(node);
        return NULL;
    }

    return node;
}

/**
 * Create field reference
 */
ast_node_t *ast_create_field_ref(const char *segment_id, int field_num) {
    if (!segment_id) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_FIELD_REF);
    if (!node) {
        return NULL;
    }

    node->field_ref.segment_id = strdup(segment_id);
    if (!node->field_ref.segment_id) {
        free(node);
        return NULL;
    }

    node->field_ref.field_num = field_num;

    return node;
}

/**
 * Create string literal
 */
ast_node_t *ast_create_literal_string(const char *value) {
    if (!value) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_LITERAL);
    if (!node) {
        return NULL;
    }

    node->literal.type = LITERAL_STRING;
    node->literal.string_value = strdup(value);

    if (!node->literal.string_value) {
        free(node);
        return NULL;
    }

    return node;
}

/**
 * Create number literal
 */
ast_node_t *ast_create_literal_number(long long value) {
    ast_node_t *node = ast_alloc(AST_LITERAL);
    if (!node) {
        return NULL;
    }

    node->literal.type = LITERAL_NUMBER;
    node->literal.int_value = value;

    return node;
}

/**
 * Create binary operation
 */
ast_node_t *ast_create_binary_op(binary_op_type_t op,
                                   ast_node_t *left,
                                   ast_node_t *right) {
    if (!left || !right) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_BINARY_OP);
    if (!node) {
        return NULL;
    }

    node->binary_op.op = op;
    node->binary_op.left = left;
    node->binary_op.right = right;

    return node;
}

/**
 * Create column
 */
ast_node_t *ast_create_column(ast_node_t *expr, const char *alias) {
    if (!expr) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_COLUMN);
    if (!node) {
        return NULL;
    }

    node->column.expr = expr;

    if (alias) {
        node->column.alias = strdup(alias);
        if (!node->column.alias) {
            free(node);
            return NULL;
        }
    } else {
        node->column.alias = NULL;
    }

    return node;
}

/**
 * Create column list
 */
ast_node_t *ast_create_column_list(ast_node_t **columns, size_t num_columns) {
    if (!columns || num_columns == 0) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_COLUMN_LIST);
    if (!node) {
        return NULL;
    }

    node->column_list.columns = calloc(num_columns, sizeof(ast_node_t *));
    if (!node->column_list.columns) {
        free(node);
        return NULL;
    }

    memcpy(node->column_list.columns, columns, num_columns * sizeof(ast_node_t *));
    node->column_list.num_columns = num_columns;

    return node;
}

/**
 * Create wildcard (*)
 */
ast_node_t *ast_create_wildcard(void) {
    return ast_alloc(AST_WILDCARD);
}

/**
 * Create COUNT aggregation
 */
ast_node_t *ast_create_count(ast_node_t *expr) {
    ast_node_t *node = ast_alloc(AST_COUNT);
    if (!node) {
        return NULL;
    }

    node->count.expr = expr; /* NULL = COUNT(*) */

    return node;
}

/**
 * Create CREATE QUEUE statement
 */
ast_node_t *ast_create_create_queue(const char *queue_name) {
    if (!queue_name) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_CREATE_QUEUE_STMT);
    if (!node) {
        return NULL;
    }

    node->create_queue_stmt.queue_name = strdup(queue_name);
    if (!node->create_queue_stmt.queue_name) {
        free(node);
        return NULL;
    }

    return node;
}

/**
 * Create DROP QUEUE statement
 */
ast_node_t *ast_create_drop_queue(const char *queue_name) {
    if (!queue_name) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_DROP_QUEUE_STMT);
    if (!node) {
        return NULL;
    }

    node->drop_queue_stmt.queue_name = strdup(queue_name);
    if (!node->drop_queue_stmt.queue_name) {
        free(node);
        return NULL;
    }

    return node;
}

/**
 * Create POP MESSAGE statement
 */
ast_node_t *ast_create_pop_message(const char *queue_name) {
    if (!queue_name) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_POP_MESSAGE_STMT);
    if (!node) {
        return NULL;
    }

    node->pop_message_stmt.queue_name = strdup(queue_name);
    if (!node->pop_message_stmt.queue_name) {
        free(node);
        return NULL;
    }

    return node;
}

/**
 * Create CLEAR HISTORY statement
 */
ast_node_t *ast_create_clear_history(const char *queue_name) {
    if (!queue_name) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_CLEAR_HISTORY_STMT);
    if (!node) {
        return NULL;
    }

    node->clear_history_stmt.queue_name = strdup(queue_name);
    if (!node->clear_history_stmt.queue_name) {
        free(node);
        return NULL;
    }

    return node;
}

/**
 * Create queue reference (named)
 */
ast_node_t *ast_create_queue_ref_named(const char *name) {
    if (!name) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_QUEUE_REF);
    if (!node) {
        return NULL;
    }

    node->queue_ref.type = QUEUE_NAMED;
    node->queue_ref.name = strdup(name);
    if (!node->queue_ref.name) {
        free(node);
        return NULL;
    }

    return node;
}

/**
 * Create queue reference (history)
 */
ast_node_t *ast_create_queue_ref_history(const char *name) {
    if (!name) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_QUEUE_REF);
    if (!node) {
        return NULL;
    }

    node->queue_ref.type = QUEUE_HISTORY;
    node->queue_ref.name = strdup(name);
    if (!node->queue_ref.name) {
        free(node);
        return NULL;
    }

    return node;
}

/**
 * Create queue reference (wildcard)
 */
ast_node_t *ast_create_queue_ref_wildcard(void) {
    ast_node_t *node = ast_alloc(AST_QUEUE_REF);
    if (!node) {
        return NULL;
    }

    node->queue_ref.type = QUEUE_WILDCARD;
    node->queue_ref.name = NULL;

    return node;
}

/**
 * Create queue reference (ALL keyword)
 */
ast_node_t *ast_create_queue_ref_all(void) {
    ast_node_t *node = ast_alloc(AST_QUEUE_REF);
    if (!node) {
        return NULL;
    }

    node->queue_ref.type = QUEUE_ALL;
    node->queue_ref.name = NULL;

    return node;
}

/**
 * Create GROUP BY clause
 */
ast_node_t *ast_create_group_by(ast_node_t **fields, size_t num_fields) {
    if (!fields || num_fields == 0) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_GROUP_BY);
    if (!node) {
        return NULL;
    }

    node->group_by.fields = calloc(num_fields, sizeof(ast_node_t *));
    if (!node->group_by.fields) {
        free(node);
        return NULL;
    }

    memcpy(node->group_by.fields, fields, num_fields * sizeof(ast_node_t *));
    node->group_by.num_fields = num_fields;

    return node;
}

/**
 * Create timestamp reference
 */
ast_node_t *ast_create_timestamp_ref(void) {
    return ast_alloc(AST_TIMESTAMP_REF);
}

/**
 * Create time function
 */
ast_node_t *ast_create_time_function(time_function_type_t func_type, int n_value) {
    if (n_value <= 0) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_TIME_FUNCTION);
    if (!node) {
        return NULL;
    }

    node->time_function.func_type = func_type;
    node->time_function.n_value = n_value;

    return node;
}

/**
 * Create BETWEEN operation
 */
ast_node_t *ast_create_between(ast_node_t *expr,
                                 ast_node_t *lower_bound,
                                 ast_node_t *upper_bound) {
    if (!expr || !lower_bound || !upper_bound) {
        return NULL;
    }

    ast_node_t *node = ast_alloc(AST_BETWEEN_OP);
    if (!node) {
        return NULL;
    }

    node->between_op.expr = expr;
    node->between_op.lower_bound = lower_bound;
    node->between_op.upper_bound = upper_bound;

    return node;
}

/**
 * Destroy AST node recursively
 */
void ast_destroy(ast_node_t *node) {
    if (!node) {
        return;
    }

    switch (node->type) {
        case AST_SELECT_STMT:
            ast_destroy(node->select_stmt.columns);
            ast_destroy(node->select_stmt.queue);
            ast_destroy(node->select_stmt.where);
            ast_destroy(node->select_stmt.group_by);
            break;

        case AST_INSERT_STMT:
            free(node->insert_stmt.queue_name);
            free(node->insert_stmt.message_data);
            break;

        case AST_CREATE_QUEUE_STMT:
            free(node->create_queue_stmt.queue_name);
            break;

        case AST_DROP_QUEUE_STMT:
            free(node->drop_queue_stmt.queue_name);
            break;

        case AST_POP_MESSAGE_STMT:
            free(node->pop_message_stmt.queue_name);
            break;

        case AST_CLEAR_HISTORY_STMT:
            free(node->clear_history_stmt.queue_name);
            break;

        case AST_QUEUE_REF:
            free(node->queue_ref.name);  /* NULL for wildcards */
            break;

        case AST_FIELD_REF:
            free(node->field_ref.segment_id);
            break;

        case AST_LITERAL:
            if (node->literal.type == LITERAL_STRING) {
                free(node->literal.string_value);
            }
            break;

        case AST_BINARY_OP:
            ast_destroy(node->binary_op.left);
            ast_destroy(node->binary_op.right);
            break;

        case AST_COLUMN:
            ast_destroy(node->column.expr);
            free(node->column.alias);
            break;

        case AST_COLUMN_LIST:
            for (size_t i = 0; i < node->column_list.num_columns; i++) {
                ast_destroy(node->column_list.columns[i]);
            }
            free(node->column_list.columns);
            break;

        case AST_COUNT:
            ast_destroy(node->count.expr);
            break;

        case AST_GROUP_BY:
            for (size_t i = 0; i < node->group_by.num_fields; i++) {
                ast_destroy(node->group_by.fields[i]);
            }
            free(node->group_by.fields);
            break;

        case AST_TIMESTAMP_REF:
            /* No data to free */
            break;

        case AST_TIME_FUNCTION:
            /* No data to free */
            break;

        case AST_BETWEEN_OP:
            ast_destroy(node->between_op.expr);
            ast_destroy(node->between_op.lower_bound);
            ast_destroy(node->between_op.upper_bound);
            break;

        case AST_WILDCARD:
            /* No data to free */
            break;
    }

    free(node);
}

/**
 * Get operator string
 */
const char *ast_get_op_string(binary_op_type_t op) {
    switch (op) {
        case OP_EQ:   return "=";
        case OP_NE:   return "!=";
        case OP_LT:   return "<";
        case OP_LE:   return "<=";
        case OP_GT:   return ">";
        case OP_GE:   return ">=";
        case OP_LIKE: return "LIKE";
        case OP_AND:  return "AND";
        case OP_OR:   return "OR";
        default:      return "UNKNOWN";
    }
}

/**
 * Print AST for debugging
 */
void ast_print(const ast_node_t *node, int indent) {
    if (!node) {
        printf("%*sNULL\n", indent, "");
        return;
    }

    switch (node->type) {
        case AST_SELECT_STMT:
            printf("%*sSELECT\n", indent, "");
            printf("%*s  Columns:\n", indent, "");
            ast_print(node->select_stmt.columns, indent + 4);
            if (node->select_stmt.queue) {
                printf("%*s  FROM:\n", indent, "");
                ast_print(node->select_stmt.queue, indent + 4);
            }
            if (node->select_stmt.where) {
                printf("%*s  WHERE:\n", indent, "");
                ast_print(node->select_stmt.where, indent + 4);
            }
            if (node->select_stmt.group_by) {
                printf("%*s  GROUP BY:\n", indent, "");
                ast_print(node->select_stmt.group_by, indent + 4);
            }
            break;

        case AST_INSERT_STMT:
            printf("%*sINSERT MESSAGE INTO '%s'\n", indent, "",
                   node->insert_stmt.queue_name);
            printf("%*s  Data: '%s'\n", indent, "",
                   node->insert_stmt.message_data);
            break;

        case AST_CREATE_QUEUE_STMT:
            printf("%*sCREATE QUEUE '%s'\n", indent, "",
                   node->create_queue_stmt.queue_name);
            break;

        case AST_DROP_QUEUE_STMT:
            printf("%*sDROP QUEUE '%s'\n", indent, "",
                   node->drop_queue_stmt.queue_name);
            break;

        case AST_POP_MESSAGE_STMT:
            printf("%*sPOP MESSAGE FROM '%s'\n", indent, "",
                   node->pop_message_stmt.queue_name);
            break;

        case AST_CLEAR_HISTORY_STMT:
            printf("%*sCLEAR HISTORY '%s'\n", indent, "",
                   node->clear_history_stmt.queue_name);
            break;

        case AST_QUEUE_REF:
            printf("%*sQUEUE_REF ", indent, "");
            switch (node->queue_ref.type) {
                case QUEUE_NAMED:
                    printf("'%s'\n", node->queue_ref.name);
                    break;
                case QUEUE_HISTORY:
                    printf("'%s.history'\n", node->queue_ref.name);
                    break;
                case QUEUE_WILDCARD:
                    printf("*\n");
                    break;
                case QUEUE_ALL:
                    printf("ALL\n");
                    break;
            }
            break;

        case AST_FIELD_REF:
            printf("%*sFIELD_REF segment.%s[%d]\n", indent, "",
                   node->field_ref.segment_id, node->field_ref.field_num);
            break;

        case AST_LITERAL:
            if (node->literal.type == LITERAL_STRING) {
                printf("%*sLITERAL '%s'\n", indent, "",
                       node->literal.string_value);
            } else {
                printf("%*sLITERAL %lld\n", indent, "",
                       node->literal.int_value);
            }
            break;

        case AST_BINARY_OP:
            printf("%*sBINARY_OP %s\n", indent, "",
                   ast_get_op_string(node->binary_op.op));
            printf("%*s  Left:\n", indent, "");
            ast_print(node->binary_op.left, indent + 4);
            printf("%*s  Right:\n", indent, "");
            ast_print(node->binary_op.right, indent + 4);
            break;

        case AST_COLUMN:
            printf("%*sCOLUMN", indent, "");
            if (node->column.alias) {
                printf(" AS %s", node->column.alias);
            }
            printf("\n");
            ast_print(node->column.expr, indent + 2);
            break;

        case AST_COLUMN_LIST:
            printf("%*sCOLUMN_LIST (%zu columns)\n", indent, "",
                   node->column_list.num_columns);
            for (size_t i = 0; i < node->column_list.num_columns; i++) {
                ast_print(node->column_list.columns[i], indent + 2);
            }
            break;

        case AST_WILDCARD:
            printf("%*sWILDCARD (*)\n", indent, "");
            break;

        case AST_COUNT:
            printf("%*sCOUNT(", indent, "");
            if (node->count.expr) {
                printf("expr)\n");
                ast_print(node->count.expr, indent + 2);
            } else {
                printf("*)\n");
            }
            break;

        case AST_GROUP_BY:
            printf("%*sGROUP_BY (%zu fields)\n", indent, "",
                   node->group_by.num_fields);
            for (size_t i = 0; i < node->group_by.num_fields; i++) {
                ast_print(node->group_by.fields[i], indent + 2);
            }
            break;

        case AST_TIMESTAMP_REF:
            printf("%*sTIMESTAMP\n", indent, "");
            break;

        case AST_TIME_FUNCTION:
            printf("%*sTIME_FUNCTION ", indent, "");
            switch (node->time_function.func_type) {
                case TIME_FUNC_LAST_N_HOURS:
                    printf("LAST_N_HOURS(%d)\n", node->time_function.n_value);
                    break;
                case TIME_FUNC_LAST_N_DAYS:
                    printf("LAST_N_DAYS(%d)\n", node->time_function.n_value);
                    break;
            }
            break;

        case AST_BETWEEN_OP:
            printf("%*sBETWEEN\n", indent, "");
            printf("%*s  Expr:\n", indent, "");
            ast_print(node->between_op.expr, indent + 4);
            printf("%*s  Lower:\n", indent, "");
            ast_print(node->between_op.lower_bound, indent + 4);
            printf("%*s  Upper:\n", indent, "");
            ast_print(node->between_op.upper_bound, indent + 4);
            break;
    }
}
