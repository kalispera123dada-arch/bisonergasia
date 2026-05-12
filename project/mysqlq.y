%{
/*
 * mysqlq.y — Συντακτικός αναλυτής (Bison) για τη ψευδογλώσσα mySQLq
 *
 * ─────────────────────────────── BNF ───────────────────────────────────────
 *
 * <πρόγραμμα>        ::= <λίστα_εντολών>
 * <λίστα_εντολών>    ::= <εντολή>
 *                      | <λίστα_εντολών> <εντολή>
 * <εντολή>           ::= <create_stmt> ';'
 *                      | <select_stmt> ';'
 *
 * <create_stmt>      ::= CREATE TABLE αναγνωριστικό '(' <λίστα_στηλών> ')'
 * <λίστα_στηλών>     ::= <ορισμός_στήλης>
 *                      | <λίστα_στηλών> ',' <ορισμός_στήλης>
 * <ορισμός_στήλης>   ::= αναγνωριστικό <τύπος_δεδομένων>
 * <τύπος_δεδομένων>  ::= INT | FLOAT | VARCHAR '(' ακέραιο ')'
 *
 * <select_stmt>      ::= SELECT <επιλογή_στηλών>
 *                        FROM <αναφορά_πίνακα>
 *                        <όροι_join>
 *                        <προαιρετικό_where>
 *                        <προαιρετικό_group>
 *                        <προαιρετικό_order>
 *                        <προαιρετικό_limit>
 *
 * <επιλογή_στηλών>   ::= '*' | <λίστα_στηλών_ref>
 * <λίστα_στηλών_ref> ::= <στήλη_ref> | <λίστα_στηλών_ref> ',' <στήλη_ref>
 * <στήλη_ref>        ::= αναγνωριστικό | αναγνωριστικό '.' αναγνωριστικό
 *
 * <αναφορά_πίνακα>   ::= αναγνωριστικό | αναγνωριστικό AS αναγνωριστικό
 *
 * <όροι_join>        ::= ε | <όροι_join> <όρος_join>
 * <όρος_join>        ::= JOIN <αναφορά_πίνακα> ON <πλήρης_στήλη> '=' <πλήρης_στήλη>
 * <πλήρης_στήλη>     ::= αναγνωριστικό '.' αναγνωριστικό | αναγνωριστικό
 *
 * <προαιρετικό_where> ::= ε | WHERE <συνθήκη>
 * <συνθήκη>           ::= <απλή_συνθήκη>
 *                       | <συνθήκη> AND <συνθήκη>
 *                       | <συνθήκη> OR  <συνθήκη>
 *                       | NOT <συνθήκη>
 *                       | '(' <συνθήκη> ')'
 * <απλή_συνθήκη>      ::= <στήλη_ref> <τελεστής> <κυριολεκτικό>
 *                       | <στήλη_ref> IN '(' <λίστα_κυριολεκτικών> ')'
 *                       | <στήλη_ref> NOT IN '(' <λίστα_κυριολεκτικών> ')'
 * <τελεστής>          ::= '=' | '!=' | '>' | '<' | '>=' | '<='
 * <κυριολεκτικό>      ::= ακέραιο | πραγματικό | αλφαριθμητικό
 * <λίστα_κυριολ.>     ::= <κυριολεκτικό> | <λίστα_κυριολ.> ',' <κυριολεκτικό>
 *
 * <προαιρετικό_group> ::= ε | GROUP BY <λίστα_στηλών_ref>
 * <προαιρετικό_order> ::= ε | ORDER BY <λίστα_στηλών_ref>
 * <προαιρετικό_limit> ::= ε | LIMIT ακέραιο   (αυστηρά θετικός)
 *
 * ────────────────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

/* ── Αποθήκευση γραμμών πηγαίου κώδικα για εκτύπωση ── */
#define MAX_LINES 4096
static char *src_lines[MAX_LINES];
static int   num_lines  = 0;

/* ── Παρακολούθηση σφαλμάτων ── */
static int   error_flag    = 0;
static int   error_line_no = 0;
static char  error_msg[256];

/* ── Ορισμοί προς τα εμπρός ── */
int  yylex(void);
void yyerror(const char *s);
extern int yylineno;
extern FILE *yyin;

/* ── Τρέχων πίνακας υπό δημιουργία (για CREATE TABLE) ── */
static Table *cur_create_table = NULL;

/* ── Λίστα στηλών SELECT για αναβαλλόμενη επικύρωση ──
   Οι στήλες του SELECT αναλύονται ΠΡΙΝ το FROM, οπότε τις
   αποθηκεύουμε και τις επικυρώνουμε αφού στηθεί το query context. */
#define MAX_PENDING 256
static char *pending_cols[MAX_PENDING];
static int   pending_cols_size = 0;

/* Καθαρίζει τη λίστα αναμονής */
static void pending_cols_clear(void)
{
    for (int i = 0; i < pending_cols_size; i++) {
        free(pending_cols[i]);
        pending_cols[i] = NULL;
    }
    pending_cols_size = 0;
}

/* Προσθέτει στήλη στη λίστα αναμονής */
static void pending_cols_add(const char *col)
{
    if (pending_cols_size < MAX_PENDING)
        pending_cols[pending_cols_size++] = strdup(col);
}

/* Επικυρώνει όλες τις αναμενόμενες στήλες του SELECT
   αφού έχει στηθεί το πλήρες query context (FROM + JOINs).
   Επιστρέφει 0 σε επιτυχία, -1 στο πρώτο σφάλμα. */
static int pending_cols_validate(void)
{
    for (int i = 0; i < pending_cols_size; i++) {
        char *cr  = pending_cols[i];
        char *dot = strchr(cr, '.');
        if (dot) {
            /* αναφορά τύπου πίνακας.στήλη */
            *dot = '\0';
            int ok = validate_qualified_column(cr, dot + 1);
            *dot = '.';
            if (ok != 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε", cr);
                strncpy(error_msg, buf, sizeof(error_msg) - 1);
                return -1;
            }
        } else {
            /* απλό όνομα στήλης */
            if (validate_column(cr) != 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε στους πίνακες του ερωτήματος", cr);
                strncpy(error_msg, buf, sizeof(error_msg) - 1);
                return -1;
            }
        }
    }
    return 0;
}

/* ── Βοηθητική συνάρτηση σημασιολογικού σφάλματος (σταματά την ανάλυση) ── */
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

/* ── Τύποι τιμών (union) ── */
%union {
    int    ival;      /* ακέραιος */
    double dval;      /* πραγματικός */
    char  *sval;      /* αλφαριθμητικό / αναγνωριστικό */
    int    lit_type;  /* κωδικός τύπου κυριολεκτικού (LIT_*) */
}

/* ── Tokens ── */
%token SELECT FROM WHERE LIMIT GROUP ORDER BY IN_KW AND OR NOT
%token CREATE TABLE INT_TYPE FLOAT_TYPE VARCHAR
%token JOIN ON AS
%token EQ NE GT LT GE LE
%token COMMA SEMICOLON LPAREN RPAREN STAR DOT

%token <ival>  INT_LIT
%token <dval>  FLOAT_LIT
%token <sval>  STRING_LIT IDENTIFIER

/* ── Προτεραιότητα τελεστών (από χαμηλότερη προς υψηλότερη) ── */
%left  OR
%left  AND
%right NOT

/* ── Τύποι μη-τερματικών ── */
%type <lit_type> literal
%type <ival>     comp_op
%type <sval>     table_ref qualified_col col_ref

%%

/* ═══════════════════════════════════════════════════════════════════════════
   Κορυφαίος κανόνας — πρόγραμμα
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
   Εντολή CREATE TABLE
   ═══════════════════════════════════════════════════════════════════════════ */

create_stmt
    : CREATE TABLE IDENTIFIER
        {
            /* Ερώτημα 2α: το όνομα του πίνακα πρέπει να είναι μοναδικό */
            if (add_table($3) != 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: ο πίνακας '%s' έχει ήδη οριστεί", $3);
                sem_error(buf);
                YYABORT;
            }
            cur_create_table = find_table($3);
            free($3);
        }
      LPAREN column_list RPAREN
        {
            cur_create_table = NULL; /* τέλος ορισμού πίνακα */
        }
    ;

column_list
    : column_def
    | column_list COMMA column_def
    ;

column_def
    : IDENTIFIER INT_TYPE
        {
            /* Ερώτημα 2α: τα ονόματα στηλών πρέπει να είναι μοναδικά εντός πίνακα */
            if (cur_create_table &&
                add_column_to_table(cur_create_table, $1, TYPE_INT, 0) != 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: διπλότυπη στήλη '%s' στον πίνακα '%s'",
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
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: διπλότυπη στήλη '%s' στον πίνακα '%s'",
                         $1, cur_create_table->name);
                sem_error(buf);
                free($1);
                YYABORT;
            }
            free($1);
        }
    | IDENTIFIER VARCHAR LPAREN INT_LIT RPAREN
        {
            /* το μέγεθος του VARCHAR πρέπει να είναι αυστηρά θετικό */
            if ($4 <= 0) {
                sem_error("Σημασιολογικό σφάλμα: το μέγεθος του VARCHAR πρέπει να είναι αυστηρά θετικό");
                free($1);
                YYABORT;
            }
            if (cur_create_table &&
                add_column_to_table(cur_create_table, $1, TYPE_VARCHAR, $4) != 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: διπλότυπη στήλη '%s' στον πίνακα '%s'",
                         $1, cur_create_table->name);
                sem_error(buf);
                free($1);
                YYABORT;
            }
            free($1);
        }
    ;

/* ═══════════════════════════════════════════════════════════════════════════
   Εντολή SELECT
   Σημείωση: το col_select αναλύεται ΠΡΙΝ το FROM, οπότε οι στήλες
   αποθηκεύονται σε λίστα αναμονής και επικυρώνονται μετά το FROM + JOIN.
   ═══════════════════════════════════════════════════════════════════════════ */

select_stmt
    : SELECT { pending_cols_clear(); } col_select
      FROM table_ref
        { free($5); /* το table_ref έχει ήδη επεξεργαστεί */ }
      join_clauses
        {
            /* Το query context είναι πλήρες — επικύρωση στηλών SELECT */
            if (pending_cols_validate() != 0) {
                error_flag    = 1;
                error_line_no = yylineno;
                /* το error_msg έχει ήδη συμπληρωθεί από την pending_cols_validate */
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
            reset_query_ctx(); /* καθαρισμός πλαισίου ερωτήματος */
        }
    ;

/* ── Επιλογή στηλών SELECT ── */

col_select
    : STAR                    /* SELECT * */
    | select_col_ref_list     /* SELECT col1, col2, ... */
    ;

/* Λίστα στηλών SELECT — αποθηκεύονται για αναβαλλόμενη επικύρωση */
select_col_ref_list
    : select_col_ref
    | select_col_ref_list COMMA select_col_ref
    ;

select_col_ref
    : IDENTIFIER
        {
            /* απλό όνομα στήλης — προσθήκη στη λίστα αναμονής */
            pending_cols_add($1);
            free($1);
        }
    | IDENTIFIER DOT IDENTIFIER
        {
            /* αναφορά τύπου πίνακας.στήλη — προσθήκη στη λίστα αναμονής */
            char buf[256];
            snprintf(buf, sizeof(buf), "%s.%s", $1, $3);
            pending_cols_add(buf);
            free($1);
            free($3);
        }
    ;

/* ── Αναφορά πίνακα (με ή χωρίς alias) ── */

table_ref
    : IDENTIFIER
        {
            /* Ερώτημα 2β: ο πίνακας πρέπει να έχει οριστεί με CREATE */
            Table *t = find_table($1);
            if (!t) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: ο πίνακας '%s' δεν έχει οριστεί", $1);
                sem_error(buf);
                free($1);
                YYABORT;
            }
            add_to_query_ctx(t, NULL);
            $$ = $1;
        }
    | IDENTIFIER AS IDENTIFIER
        {
            /* Ερώτημα 3β: ψευδώνυμο πίνακα */
            Table *t = find_table($1);
            if (!t) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: ο πίνακας '%s' δεν έχει οριστεί", $1);
                sem_error(buf);
                free($1); free($3);
                YYABORT;
            }
            if (add_to_query_ctx(t, $3) != 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: το alias '%s' χρησιμοποιείται ήδη", $3);
                sem_error(buf);
                free($1); free($3);
                YYABORT;
            }
            $$ = $1;
            free($3);
        }
    ;

/* ── Όροι JOIN ── */

join_clauses
    : /* κενό — δεν υπάρχει JOIN */
    | join_clauses join_clause
    ;

join_clause
    : JOIN table_ref
        { free($2); }
      ON qualified_col EQ qualified_col
        {
            char *left  = $5; /* αριστερή στήλη του ON */
            char *right = $7; /* δεξιά στήλη του ON */

            /* επικύρωση αριστερής στήλης */
            char *dot = strchr(left, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(left, dot + 1) != 0) {
                    char buf[256]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε", left);
                    sem_error(buf); free(left); free(right); YYABORT;
                }
                *dot = '.';
            } else {
                if (validate_column(left) != 0) {
                    char buf[256];
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε", left);
                    sem_error(buf); free(left); free(right); YYABORT;
                }
            }

            /* επικύρωση δεξιάς στήλης */
            dot = strchr(right, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(right, dot + 1) != 0) {
                    char buf[256]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε", right);
                    sem_error(buf); free(left); free(right); YYABORT;
                }
                *dot = '.';
            } else {
                if (validate_column(right) != 0) {
                    char buf[256];
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε", right);
                    sem_error(buf); free(left); free(right); YYABORT;
                }
            }

            free(left); free(right);
        }
    ;

/* Πλήρης αναφορά στήλης (πίνακας.στήλη ή απλό όνομα) */
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

/* ── Όρος WHERE ── */

opt_where
    : /* κενό */
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
            /* Ερώτημα 2ε: έλεγχος ύπαρξης στήλης και συμβατότητας τύπων */
            char *cr  = $1;
            int   lt  = $3;
            ColType ct;
            char *dot = strchr(cr, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(cr, dot + 1) != 0) {
                    char buf[256]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                ct = qualified_column_type(cr, dot + 1);
                *dot = '.';
            } else {
                if (validate_column(cr) != 0) {
                    char buf[256];
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε στους πίνακες του ερωτήματος", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                ct = column_type(cr);
            }
            if (!types_compatible(ct, lt)) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: ασυμβατότητα τύπου για τη στήλη '%s'", cr);
                sem_error(buf); free(cr); YYABORT;
            }
            free(cr);
        }
    | col_ref IN_KW LPAREN lit_list RPAREN
        {
            /* Ερώτημα 2ε: έλεγχος ύπαρξης στήλης για τελεστή IN */
            char *cr  = $1;
            char *dot = strchr(cr, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(cr, dot + 1) != 0) {
                    char buf[256]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                *dot = '.';
            } else {
                if (validate_column(cr) != 0) {
                    char buf[256];
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε στους πίνακες του ερωτήματος", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
            }
            free(cr);
        }
    | col_ref NOT IN_KW LPAREN lit_list RPAREN
        {
            /* Ερώτημα 2ε: έλεγχος ύπαρξης στήλης για τελεστή NOT IN */
            char *cr  = $1;
            char *dot = strchr(cr, '.');
            if (dot) {
                *dot = '\0';
                if (validate_qualified_column(cr, dot + 1) != 0) {
                    char buf[256]; *dot = '.';
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
                *dot = '.';
            } else {
                if (validate_column(cr) != 0) {
                    char buf[256];
                    snprintf(buf, sizeof(buf),
                             "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε στους πίνακες του ερωτήματος", cr);
                    sem_error(buf); free(cr); YYABORT;
                }
            }
            free(cr);
        }
    ;

/* Αναφορά στήλης στο WHERE/JOIN — επιστρέφει malloc'd string */
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

lit_list
    : literal
    | lit_list COMMA literal
    ;

/* ── Τελεστές σύγκρισης ── */

comp_op
    : EQ  { $$ = 0; }  /* = */
    | NE  { $$ = 1; }  /* != */
    | GT  { $$ = 2; }  /* > */
    | LT  { $$ = 3; }  /* < */
    | GE  { $$ = 4; }  /* >= */
    | LE  { $$ = 5; }  /* <= */
    ;

/* Κυριολεκτικά — επιστρέφουν κωδικό τύπου LIT_* */
literal
    : INT_LIT    { $$ = LIT_INT;    }
    | FLOAT_LIT  { $$ = LIT_FLOAT;  }
    | STRING_LIT { free($1); $$ = LIT_STRING; }
    ;

/* ── Όρος GROUP BY ── */

opt_group
    : /* κενό */
    | GROUP BY validated_col_ref_list
    ;

/* ── Όρος ORDER BY ── */

opt_order
    : /* κενό */
    | ORDER BY validated_col_ref_list
    ;

/* Λίστα στηλών με άμεση επικύρωση — χρησιμοποιείται στο GROUP BY / ORDER BY */
validated_col_ref_list
    : validated_col_ref
    | validated_col_ref_list COMMA validated_col_ref
    ;

validated_col_ref
    : IDENTIFIER
        {
            /* Ερώτημα 2δ: η στήλη πρέπει να υπάρχει στους πίνακες του ερωτήματος */
            if (validate_column($1) != 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: η στήλη '%s' δεν βρέθηκε στους πίνακες του ερωτήματος", $1);
                sem_error(buf); free($1); YYABORT;
            }
            free($1);
        }
    | IDENTIFIER DOT IDENTIFIER
        {
            if (validate_qualified_column($1, $3) != 0) {
                char buf[256];
                snprintf(buf, sizeof(buf),
                         "Σημασιολογικό σφάλμα: η στήλη '%s.%s' δεν βρέθηκε", $1, $3);
                sem_error(buf); free($1); free($3); YYABORT;
            }
            free($1); free($3);
        }
    ;

/* ── Όρος LIMIT ── */

opt_limit
    : /* κενό */
    | LIMIT INT_LIT
        {
            /* το LIMIT πρέπει να είναι αυστηρά θετικός ακέραιος */
            if ($2 <= 0) {
                sem_error("Σημασιολογικό σφάλμα: η τιμή του LIMIT πρέπει να είναι αυστηρά θετική");
                YYABORT;
            }
        }
    ;

%%

/* ═══════════════════════════════════════════════════════════════════════════
   yyerror — καλείται αυτόματα από τον Bison σε συντακτικό σφάλμα
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
   print_lines — εκτυπώνει γραμμές πηγαίου κώδικα από 1 έως upto (συμπεριλαμβ.)
   ═══════════════════════════════════════════════════════════════════════════ */
static void print_lines(int upto)
{
    int limit = (upto < num_lines) ? upto : num_lines;
    for (int i = 0; i < limit; i++)
        printf("%s", src_lines[i]);
}

/* ═══════════════════════════════════════════════════════════════════════════
   main — σημείο εισόδου του προγράμματος
   Χρήση: myParser <αρχείο_εισόδου>
   ═══════════════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Χρήση: myParser <αρχείο>\n");
        return 1;
    }

    /* ── Ανάγνωση όλων των γραμμών του πηγαίου αρχείου ── */
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    char buf[4096];
    while (num_lines < MAX_LINES && fgets(buf, sizeof(buf), f))
        src_lines[num_lines++] = strdup(buf);
    fclose(f);

    /* ── Εκτέλεση ανάλυσης ── */
    f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    yyin = f;
    int parse_result = yyparse();
    fclose(f);

    /* ── Εμφάνιση αποτελέσματος ── */
    if (!error_flag && parse_result == 0) {
        /* Επιτυχία: εκτύπωση ολόκληρου του προγράμματος + μήνυμα */
        print_lines(num_lines);
        printf("\n--- Συντακτικά και σημασιολογικά ορθό ---\n");
        return 0;
    } else {
        /* Σφάλμα: εκτύπωση μέχρι τη γραμμή σφάλματος + μήνυμα */
        int el = (error_line_no > 0) ? error_line_no : 1;
        print_lines(el);
        fprintf(stderr, "\nΣφάλμα στη γραμμή %d: %s\n", el, error_msg);
        return 1;
    }
}
