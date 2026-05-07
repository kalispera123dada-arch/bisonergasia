#ifndef SYMTABLE_H
#define SYMTABLE_H

#define MAX_TABLES   128
#define MAX_COLUMNS  128
#define MAX_ALIASES  32

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_VARCHAR
} ColType;

typedef struct Column {
    char     *name;
    ColType   type;
    int       varchar_size;
    struct Column *next;
} Column;

typedef struct Table {
    char    *name;
    Column  *columns;
    struct Table *next;
} Table;

/* Global symbol table (linked list of tables) */
extern Table *symbol_table;

/* Symbol table operations */
Table  *find_table(const char *name);
int     add_table(const char *name);          /* 0=ok, -1=duplicate */
Column *find_column_in_table(Table *t, const char *colname);
int     add_column_to_table(Table *t, const char *colname,
                             ColType type, int varchar_size); /* 0=ok, -1=dup */

/* Query context: tracks tables/aliases in scope for the current SELECT */
typedef struct {
    Table  *table;
    char   *alias;   /* NULL if none */
} TableRef;

extern TableRef query_ctx[MAX_ALIASES];
extern int      query_ctx_size;

void    reset_query_ctx(void);
int     add_to_query_ctx(Table *t, const char *alias); /* 0=ok, -1=dup alias */

/* Resolve a table name OR alias to a Table* (returns NULL if not found) */
Table  *resolve_name(const char *name);

/* Validate a plain column name exists in exactly one table in query context */
int     validate_column(const char *colname);  /* 0=ok, -1=not found */

/* Validate table_name.col_name — table_name may be an alias */
int     validate_qualified_column(const char *tblname, const char *colname);

/* Type of a column in current query context (first match) */
ColType column_type(const char *colname);
ColType qualified_column_type(const char *tblname, const char *colname);

/* Literal type codes (returned by flex actions) */
#define LIT_INT     1
#define LIT_FLOAT   2
#define LIT_STRING  3

int     types_compatible(ColType coltype, int lit_type); /* 0=no, 1=yes */

#endif /* SYMTABLE_H */
