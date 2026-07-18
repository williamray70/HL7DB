/**
 * hl7sql.y - Bison Parser for HL7SQL
 *
 * Defines the grammar for HL7SQL and builds Abstract Syntax Trees.
 */

%{
#include "query/hl7sql_ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External from lexer */
extern int yylex(void);
extern int yylineno;
extern char *yytext;

/* Error reporting */
void yyerror(ast_node_t **result, const char *msg) {
    (void)result;  /* Unused parameter */
    fprintf(stderr, "Parse error at line %d: %s (near '%s')\n",
            yylineno, msg, yytext ? yytext : "(null)");
    fflush(stderr);
}

%}

/* Parse result - pointer to AST root */
%parse-param {ast_node_t **result}

/* Union for semantic values */
%union {
    char *string;
    long long number;
    ast_node_t *node;
    ast_node_t **node_list;
    int count;
}

/* Terminal symbols (tokens from lexer) */
%token SELECT FROM WHERE INSERT MESSAGE AND OR LIKE BETWEEN IN
%token COUNT GROUP BY AS
%token CREATE DROP QUEUE INTO ALL POP CLEAR HISTORY
%token SEGMENT
%token TIMESTAMP LAST_N_HOURS LAST_N_DAYS
%token EQ NE LT LE GT GE
%token LPAREN RPAREN LBRACKET RBRACKET COMMA DOT STAR SEMICOLON
%token ERROR_TOKEN

%token <string> STRING_LITERAL IDENTIFIER
%token <number> NUMBER

/* Non-terminal symbols with types */
%type <node> statement select_stmt insert_stmt
%type <node> create_queue_stmt drop_queue_stmt pop_message_stmt clear_history_stmt queue_ref
%type <node> column_list column_spec expression
%type <node> where_clause condition comparison
%type <node> field_ref aggregate_func time_function

/* Operator precedence (lowest to highest) */
%left OR
%left AND
%left EQ NE LT LE GT GE LIKE
%left DOT

%%

/* Top-level statement */
statement:
    select_stmt SEMICOLON           { *result = $1; }
  | select_stmt                     { *result = $1; }
  | insert_stmt SEMICOLON           { *result = $1; }
  | insert_stmt                     { *result = $1; }
  | create_queue_stmt SEMICOLON   { *result = $1; }
  | create_queue_stmt             { *result = $1; }
  | drop_queue_stmt SEMICOLON     { *result = $1; }
  | drop_queue_stmt               { *result = $1; }
  | pop_message_stmt SEMICOLON      { *result = $1; }
  | pop_message_stmt                { *result = $1; }
  | clear_history_stmt SEMICOLON    { *result = $1; }
  | clear_history_stmt              { *result = $1; }
  ;

/* SELECT statement */
select_stmt:
    SELECT column_list FROM queue_ref where_clause {
        $$ = ast_create_select($2, $4, $5, NULL);
    }
  ;

/* Column list */
column_list:
    STAR {
        $$ = ast_create_wildcard();
    }
  | column_spec {
        ast_node_t *cols[1] = { $1 };
        $$ = ast_create_column_list(cols, 1);
    }
  | column_list COMMA column_spec {
        /* Append to existing column list */
        ast_column_list_t *list = &($1->column_list);
        size_t new_size = list->num_columns + 1;
        ast_node_t **new_cols = realloc(list->columns,
                                         new_size * sizeof(ast_node_t *));
        if (new_cols) {
            new_cols[new_size - 1] = $3;
            list->columns = new_cols;
            list->num_columns = new_size;
        }
        $$ = $1;
    }
  ;

/* Column specification (with optional alias) */
column_spec:
    expression {
        $$ = ast_create_column($1, NULL);
    }
  | expression AS IDENTIFIER {
        $$ = ast_create_column($1, $3);
        free($3);
    }
  ;

/* Expression */
expression:
    field_ref               { $$ = $1; }
  | aggregate_func          { $$ = $1; }
  | time_function           { $$ = $1; }
  | TIMESTAMP               { $$ = ast_create_timestamp_ref(); }
  | STRING_LITERAL          {
        $$ = ast_create_literal_string($1);
        free($1);
    }
  | NUMBER                  { $$ = ast_create_literal_number($1); }
  ;

/* Aggregate functions */
aggregate_func:
    COUNT LPAREN STAR RPAREN {
        $$ = ast_create_count(NULL);  /* COUNT(*) */
    }
  | COUNT LPAREN expression RPAREN {
        $$ = ast_create_count($3);
    }
  ;

/* Time functions */
time_function:
    LAST_N_HOURS LPAREN NUMBER RPAREN {
        $$ = ast_create_time_function(TIME_FUNC_LAST_N_HOURS, (int)$3);
    }
  | LAST_N_DAYS LPAREN NUMBER RPAREN {
        $$ = ast_create_time_function(TIME_FUNC_LAST_N_DAYS, (int)$3);
    }
  ;

/* Field reference: segment.SEGMENT_ID[field_num] */
field_ref:
    SEGMENT DOT IDENTIFIER LBRACKET NUMBER RBRACKET {
        $$ = ast_create_field_ref($3, (int)$5);
        free($3);
    }
  ;

/* WHERE clause */
where_clause:
    /* empty */                 { $$ = NULL; }
  | WHERE condition             { $$ = $2; }
  ;

/* Condition (boolean expression) - simplified */
condition:
    comparison                  { $$ = $1; }
  | condition AND comparison    { $$ = ast_create_binary_op(OP_AND, $1, $3); }
  | condition OR comparison     { $$ = ast_create_binary_op(OP_OR, $1, $3); }
  | LPAREN condition RPAREN     { $$ = $2; }
  ;

/* Comparison expression */
comparison:
    expression EQ expression    { $$ = ast_create_binary_op(OP_EQ, $1, $3); }
  | expression NE expression    { $$ = ast_create_binary_op(OP_NE, $1, $3); }
  | expression LT expression    { $$ = ast_create_binary_op(OP_LT, $1, $3); }
  | expression LE expression    { $$ = ast_create_binary_op(OP_LE, $1, $3); }
  | expression GT expression    { $$ = ast_create_binary_op(OP_GT, $1, $3); }
  | expression GE expression    { $$ = ast_create_binary_op(OP_GE, $1, $3); }
  | expression LIKE expression  { $$ = ast_create_binary_op(OP_LIKE, $1, $3); }
  | expression BETWEEN expression AND expression {
        $$ = ast_create_between($1, $3, $5);
    }
  ;

/* INSERT statement */
insert_stmt:
    INSERT MESSAGE INTO IDENTIFIER STRING_LITERAL {
        $$ = ast_create_insert($4, $5);
        free($4);
        free($5);
    }
  ;

/* CREATE QUEUE statement */
create_queue_stmt:
    CREATE QUEUE IDENTIFIER {
        $$ = ast_create_create_queue($3);
        free($3);
    }
  ;

/* DROP QUEUE statement */
drop_queue_stmt:
    DROP QUEUE IDENTIFIER {
        $$ = ast_create_drop_queue($3);
        free($3);
    }
  ;

/* POP MESSAGE statement */
pop_message_stmt:
    POP MESSAGE FROM IDENTIFIER {
        $$ = ast_create_pop_message($4);
        free($4);
    }
  ;

/* CLEAR HISTORY statement */
clear_history_stmt:
    CLEAR HISTORY IDENTIFIER {
        $$ = ast_create_clear_history($3);
        free($3);
    }
  ;

/* Queue reference */
queue_ref:
    IDENTIFIER {
        $$ = ast_create_queue_ref_named($1);
        free($1);
    }
  | IDENTIFIER DOT HISTORY {
        $$ = ast_create_queue_ref_history($1);
        free($1);
    }
  | STAR {
        $$ = ast_create_queue_ref_wildcard();
    }
  | ALL {
        $$ = ast_create_queue_ref_all();
    }
  ;

%%

/* Additional C code */
