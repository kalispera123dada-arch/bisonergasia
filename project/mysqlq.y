%{
/*
 * mysqlq.y — Bison parser for the mySQLq pseudo-SQL language
 *
 *
 * ─────────────────────────────── BNF ───────────────────────────────────────
 *
 * <program>         ::= <statement_list>
 * <statement_list>  ::= <statement>
 *                     | <statement_list> <statement>
 * <statement>       ::= <create_stmt> ';'
 *                     | <select_stmt> ';'
 *
 * <create_stmt>     ::= CREATE TABLE identifier '(' <column_list> ')'
 * <column_list>     ::= <column_def>
 *                     | <column_list> ',' <column_def>
 * <column_def>      ::= identifier <data_type>
 * <data_type>       ::= INT
 *                     | FLOAT
 *                     | VARCHAR '(' int_literal ')'
 *
 * <select_stmt>     ::= SELECT <col_select>
 *                       FROM <table_ref>
 *                       <join_clauses>
 *                       <opt_where>
 *                       <opt_group>
 *                       <opt_order>
 *                       <opt_limit>
 *
 * <col_select>      ::= '*'
 *                     | <col_ref_list>
 * <col_ref_list>    ::= <col_ref>
 *                     | <col_ref_list> ',' <col_ref>
 * <col_ref>         ::= identifier
 *                     | identifier '.' identifier
 *
 * <table_ref>       ::= identifier
 *                     | identifier AS identifier
 *
 * <join_clauses>    ::= ε
 *                     | <join_clauses> <join_clause>
 * <join_clause>     ::= JOIN <table_ref> ON <qualified_col> '=' <qualified_col>
 * <qualified_col>   ::= identifier '.' identifier
 *                     | identifier
 *
 * <opt_where>       ::= ε
 *                     | WHERE <condition>
 * <condition>       ::= <simple_cond>
 *                     | <condition> AND <condition>
 *                     | <condition> OR  <condition>
 *                     | NOT <condition>
 *                     | '(' <condition> ')'
 * <simple_cond>     ::= <col_ref> <comp_op> <literal>
 *                     | <col_ref> IN '(' <lit_list> ')'
 *                     | <col_ref> NOT IN '(' <lit_list> ')'
 * <comp_op>         ::= '=' | '!=' | '>' | '<' | '>=' | '<='
 * <literal>         ::= int_literal | float_literal | string_literal
 * <lit_list>        ::= <literal>
 *                     | <lit_list> ',' <literal>
 *
 * <opt_group>       ::= ε
 *                     | GROUP BY <col_ref_list>
 * <opt_order>       ::= ε
 *                     | ORDER BY <col_ref_list>
 * <opt_limit>       ::= ε
 *                     | LIMIT int_literal        (must be strictly positive)
 *
 * ────────────────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

/* ── line storage for pretty output ───────────────────────────────────────── */
#define MAX_LINES 4096
static char *src_lines[MAX_LINES];
static int   num_lines  = 0;

/* ── error tracking ──────────────────────────────────────────────────────── */
static int   error_flag = 0;
static int   error_line_no = 0;
static char  error_msg[256];

/* ── forward declarations ────────────────────────────────────────────────── */
int  yylex(void);
void yyerror(const char *s);
extern int yylineno;
extern FILE *yyin;

/* ── current CREATE table ────────────────────────────────────────────────── */
static Table *cur_create_table = NULL;

/* ── deferred SELECT column validation list ──────────────────────────────── */
#define MAX_PENDING 256
static char *pending_cols[MAX_PENDING];
static int   pending_cols_size = 0;

static void pending_cols_clear(void)
{
    for (int i = 0; i < pending_cols_size; i++) {
        free(pending_cols[i]);
        pending_cols[i] = NULL;
    }
    pending_cols_size = 0;
}

static void pending_cols_add(const char *col)
{
    if (pending_cols_size < MAX_PENDING)
        pending_cols[pending_cols_size++] = strdup(col);
}

/* Validate all pending SELECT columns against current query context.
   Returns 0 on success, -1 on first error (fills error_msg). */
static int pending_cols_validate(void)
{
    for (int i = 0; i < pending_cols_size; i++) {
        char *cr  = pending_cols[i];
        char *dot = strchr(cr, '.');
        if (dot) {
            *dot = '\0';
            int ok = validate_qualified_column(cr, dot + 1);
            *dot = '.';
            if (ok != 0) {
                char buf[160];
                snprintf(buf, sizeof(buf),
                         "Semantic error: column '%s' not found", cr);
                strncpy(error_msg, buf, sizeof(error_msg) - 1);
                return -1;
            }
        } else {
            if (validate_column(cr) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: column '%s' not found in query tables", cr);
                strncpy(error_msg, buf, sizeof(error_msg) - 1);
                return -1;
            }
        }
    }
    return 0;
}

/* ── helper: semantic error (stops parse) ───────────────────────────────── */
static void sem_error(const char *msg)
{
    if (!error_flag) {
        error_flag    = 1;
        error_line_no = yylineno;
        strncpy(error_msg, msg, sizeof(error_msg) - 1);
        error_msg[sizeof(error_msg) - 1] = '\0';
    }
}

%}

/* ── value union ─────────────────────────────────────────────────────────── */
%union {
    int    ival; 
    double dval;
    char  *sval;
    int    lit_type;
}

/* ── tokens ──────────────────────────────────────────────────────────────── */
%token SELECT FROM WHERE LIMIT GROUP ORDER BY IN_KW AND OR NOT
%token CREATE TABLE INT_TYPE FLOAT_TYPE VARCHAR
%token JOIN ON AS
%token EQ NE GT LT GE LE
%token COMMA SEMICOLON LPAREN RPAREN STAR DOT

%token <ival>  INT_LIT
%token <dval>  FLOAT_LIT
%token <sval>  STRING_LIT IDENTIFIER

/* ── operator precedence (lowest → highest) ──────────────────────────────── */
%left  OR
%left  AND
%right NOT

/* ── typed non-terminals ─────────────────────────────────────────────────── */
%type <lit_type> literal lit_list
%type <ival>     comp_op
%type <sval>     table_ref qualified_col col_ref

%%

/* ═══════════════════════════════════════════════════════════════════════════
   Top-level program
   ═══════════════════════════════════════════════════════════════════════════ */

program
    : statement_list
    ;

statement_list
    : statement
    | statement_list statement
    ;

statement
    : create_stmt SEMICOLON
    | select_stmt SEMICOLON
    ;

/* ═══════════════════════════════════════════════════════════════════════════
   CREATE TABLE statement
   ═══════════════════════════════════════════════════════════════════════════ */

create_stmt
    : CREATE TABLE IDENTIFIER
        {
            /* Q2a: table name must be unique */
            if (add_table($3) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: table '%s' already defined", $3);
                sem_error(buf);
                YYABORT;
            }
            cur_create_table = find_table($3);
            free($3);
        }
      LPAREN column_list RPAREN
        {
            cur_create_table = NULL;
        }
    ;

column_list
    : column_def
    | column_list COMMA column_def
    ;

column_def
    : IDENTIFIER INT_TYPE
        {
            if (cur_create_table &&
                add_column_to_table(cur_create_table, $1, TYPE_INT, 0) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: duplicate column '%s' in table '%s'",
                         $1, cur_create_table->name);
                sem_error(buf);
                free($1);
                YYABORT;
            }
            free($1);
        }
    | IDENTIFIER FLOAT_TYPE
        {
            if (cur_create_table &&
                add_column_to_table(cur_create_table, $1, TYPE_FLOAT, 0) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: duplicate column '%s' in table '%s'",
                         $1, cur_create_table->name);
                sem_error(buf);
                free($1);
                YYABORT;
            }
            free($1);
        }
    | IDENTIFIER VARCHAR LPAREN INT_LIT RPAREN
        {
            if ($4 <= 0) {
                sem_error("Semantic error: VARCHAR size must be strictly positive");
                free($1);
                YYABORT;
            }
            if (cur_create_table &&
                add_column_to_table(cur_create_table, $1, TYPE_VARCHAR, $4) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: duplicate column '%s' in table '%s'",
                         $1, cur_create_table->name);
                sem_error(buf);
                free($1);
                YYABORT;
            }
            free($1);
        }
    ;

/* ═══════════════════════════════════════════════════════════════════════════
   SELECT statement
   Note: col_select is parsed BEFORE FROM, so we store SELECT columns in a
   pending list and validate them AFTER the query context is fully set up
   (after FROM + JOIN clauses).
   ═══════════════════════════════════════════════════════════════════════════ */

select_stmt
    : SELECT { pending_cols_clear(); } col_select
      FROM table_ref
        { free($5); }
      join_clauses
        {
            /* Now query context is complete — validate deferred SELECT cols */
            if (pending_cols_validate() != 0) {
                error_flag    = 1;
                error_line_no = yylineno;
                /* error_msg already filled by pending_cols_validate */
                pending_cols_clear();
                YYABORT;
            }
            pending_cols_clear();
        }
      opt_where
      opt_group
      opt_order
      opt_limit
        {
            reset_query_ctx();
        }
    ;

/* ── col_select ─────────────────────────────────────────────────────────── */

col_select
    : STAR
    | select_col_ref_list
    ;

/* SELECT column list — columns stored in pending list, validated later */
select_col_ref_list
    : select_col_ref
    | select_col_ref_list COMMA select_col_ref
    ;

select_col_ref
    : IDENTIFIER
        {
            pending_cols_add($1);
            free($1);
        }
    | IDENTIFIER DOT IDENTIFIER
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s.%s", $1, $3);
            pending_cols_add(buf);
            free($1);
            free($3);
        }
    ;

/* ── table_ref ──────────────────────────────────────────────────────────── */

table_ref
    : IDENTIFIER
        {
            /* Q2b: table must have been CREATEd */
            Table *t = find_table($1);
            if (!t) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: table '%s' not defined", $1);
                sem_error(buf);
                free($1);
                YYABORT;
            }
            add_to_query_ctx(t, NULL);
            $$ = $1;
        }
    | IDENTIFIER AS IDENTIFIER
        {
            /* Q3b: alias */
            Table *t = find_table($1);
            if (!t) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: table '%s' not defined", $1);
                sem_error(buf);
                free($1); free($3);
                YYABORT;
            }
            if (add_to_query_ctx(t, $3) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: alias '%s' already in use", $3);
                sem_error(buf);
                free($1); free($3);
                YYABORT;
            }
            $$ = $1;
            free($3);
        }
    ;

/* ── JOIN clauses ─────────────────────────────────────────────────────────── */

join_clauses
    : /* empty */
    | join_clauses join_clause
    ;

join_clause
    : JOIN table_ref
        { free($2); }
      ON qualified_col EQ qualified_col
        {
            char *left  = $5;
            char *right = $7;

            /* validate left side */
            char *dot = strchr(left, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(left, dot + 1) != 0) {
                    char buf[160]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found", left);
                    sem_error(buf); free(left); free(right); YYABORT;
                }
                *dot = '.';
            } else {
                if (validate_column(left) != 0) {
                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found", left);
                    sem_error(buf); free(left); free(right); YYABORT;
                }
            }

            /* validate right side */
            dot = strchr(right, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(right, dot + 1) != 0) {
                    char buf[160]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found", right);
                    sem_error(buf); free(left); free(right); YYABORT;
                }
                *dot = '.';
            } else {
                if (validate_column(right) != 0) {
                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found", right);
                    sem_error(buf); free(left); free(right); YYABORT;
                }
            }

            free(left); free(right);
        }
    ;

qualified_col
    : IDENTIFIER DOT IDENTIFIER
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s.%s", $1, $3);
            $$ = strdup(buf);
            free($1); free($3);
        }
    | IDENTIFIER
        { $$ = $1; }
    ;

/* ── WHERE condition ─────────────────────────────────────────────────────── */

opt_where
    : /* empty */
    | WHERE condition
    ;

condition
    : simple_cond
    | condition AND condition
    | condition OR  condition
    | NOT condition
    | LPAREN condition RPAREN
    ;

simple_cond
    : col_ref comp_op literal
        {
            /* Q2e: type compatibility check */
            char *cr  = $1;
            int   lt  = $3;
            ColType ct;
            char *dot = strchr(cr, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(cr, dot + 1) != 0) {
                    char buf[128]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                ct = qualified_column_type(cr, dot + 1);
                *dot = '.';
            } else {
                if (validate_column(cr) != 0) {
                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found in query tables", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                ct = column_type(cr);
            }
            if (!types_compatible(ct, lt)) {
                char buf[160];
                snprintf(buf, sizeof(buf),
                         "Semantic error: type mismatch for column '%s'", cr);
                sem_error(buf); free(cr); YYABORT;
            }
            free(cr);
        }
    | col_ref IN_KW LPAREN lit_list RPAREN
        {
            char *cr  = $1;
            int   lt  = $4;
            ColType ct;
            char *dot = strchr(cr, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(cr, dot + 1) != 0) {
                    char buf[128]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                ct = qualified_column_type(cr, dot + 1);
                *dot = '.';
            } else {
                if (validate_column(cr) != 0) {
                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found in query tables", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                ct = column_type(cr);
            }
            if (lt == -1 || !types_compatible(ct, lt)) {
                char buf[160];
                snprintf(buf, sizeof(buf),
                         "Semantic error: type mismatch in IN list for column '%s'", cr);
                sem_error(buf); free(cr); YYABORT;
            }
            free(cr);
        }
    | col_ref NOT IN_KW LPAREN lit_list RPAREN
        {
            char *cr  = $1;
            int   lt  = $5;
            ColType ct;
            char *dot = strchr(cr, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(cr, dot + 1) != 0) {
                    char buf[128]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                ct = qualified_column_type(cr, dot + 1);
                *dot = '.';
            } else {
                if (validate_column(cr) != 0) {
                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Semantic error: column '%s' not found in query tables", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                ct = column_type(cr);
            }
            if (lt == -1 || !types_compatible(ct, lt)) {
                char buf[160];
                snprintf(buf, sizeof(buf),
                         "Semantic error: type mismatch in NOT IN list for column '%s'", cr);
                sem_error(buf); free(cr); YYABORT;
            }
            free(cr);
        }
    ;

/* col_ref used in WHERE / JOIN ON — returns malloc'd string */
col_ref
    : IDENTIFIER
        { $$ = $1; }
    | IDENTIFIER DOT IDENTIFIER
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s.%s", $1, $3);
            $$ = strdup(buf);
            free($1); free($3);
        }
    ;

/* lit_list returns the type of the first literal.
   Mixed types (e.g. int and string) are flagged via LIT_MIXED=-1 */
lit_list
    : literal
        { $$ = $1; }
    | lit_list COMMA literal
        {
            /* if types differ, mark as mixed (-1) */
            $$ = ($1 == $3) ? $1 : -1;
        }
    ;

/* ── comparison operators ────────────────────────────────────────────────── */

comp_op
    : EQ  { $$ = 0; }
    | NE  { $$ = 1; }
    | GT  { $$ = 2; }
    | LT  { $$ = 3; }
    | GE  { $$ = 4; }
    | LE  { $$ = 5; }
    ;

/* literal returns a LIT_* type code */
literal
    : INT_LIT    { $$ = LIT_INT;    }
    | FLOAT_LIT  { $$ = LIT_FLOAT;  }
    | STRING_LIT { free($1); $$ = LIT_STRING; }
    ;

/* ── GROUP BY ─────────────────────────────────────────────────────────────── */

opt_group
    : /* empty */
    | GROUP BY validated_col_ref_list
    ;

/* ── ORDER BY ─────────────────────────────────────────────────────────────── */

opt_order
    : /* empty */
    | ORDER BY validated_col_ref_list
    ;

/* validated col_ref_list — used in GROUP BY / ORDER BY */
validated_col_ref_list
    : validated_col_ref
    | validated_col_ref_list COMMA validated_col_ref
    ;

validated_col_ref
    : IDENTIFIER
        {
            if (validate_column($1) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: column '%s' not found in query tables", $1);
                sem_error(buf); free($1); YYABORT;
            }
            free($1);
        }
    | IDENTIFIER DOT IDENTIFIER
        {
            if (validate_qualified_column($1, $3) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: column '%s.%s' not found", $1, $3);
                sem_error(buf); free($1); free($3); YYABORT;
            }
            free($1); free($3);
        }
    ;

/* ── LIMIT ───────────────────────────────────────────────────────────────── */

opt_limit
    : /* empty */
    | LIMIT INT_LIT
        {
            if ($2 <= 0) {
                sem_error("Semantic error: LIMIT value must be strictly positive");
                YYABORT;
            }
        }
    ;

%%

/* ═══════════════════════════════════════════════════════════════════════════
   yyerror
   ═══════════════════════════════════════════════════════════════════════════ */
void yyerror(const char *s)
{
    if (!error_flag) {
        error_flag    = 1;
        error_line_no = yylineno;
        strncpy(error_msg, s, sizeof(error_msg) - 1);
        error_msg[sizeof(error_msg) - 1] = '\0';
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   print_lines — print source lines 1..upto (1-based inclusive)
   ═══════════════════════════════════════════════════════════════════════════ */
static void print_lines(int upto)
{
    int limit = (upto < num_lines) ? upto : num_lines;
    for (int i = 0; i < limit; i++)
        printf("%s", src_lines[i]);
}

/* ═══════════════════════════════════════════════════════════════════════════
   main
   ═══════════════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: myParser file_name\n");
        return 1;
    }

    /* read all source lines */
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    char buf[4096];
    while (num_lines < MAX_LINES && fgets(buf, sizeof(buf), f))
        src_lines[num_lines++] = strdup(buf);
    fclose(f);

    /* parse */
    f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    yyin = f;
    int parse_result = yyparse();
    fclose(f);

    /* output */
    if (!error_flag && parse_result == 0) {
        print_lines(num_lines);
        printf("\n--- Syntax and semantics OK ---\n");
        return 0;
    } else {
        int el = (error_line_no > 0) ? error_line_no : 1;
        print_lines(el);
        fprintf(stderr, "\nError at line %d: %s\n", el, error_msg);
        return 1;
    }
}
