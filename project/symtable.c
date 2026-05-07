#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

Table   *symbol_table   = NULL;
TableRef query_ctx[MAX_ALIASES];
int      query_ctx_size = 0;

/* ── symbol table ────────────────────────────────────────────────────────── */

Table *find_table(const char *name)
{
    for (Table *t = symbol_table; t; t = t->next)
        if (strcmp(t->name, name) == 0) return t;
    return NULL;
}

int add_table(const char *name)
{
    if (find_table(name)) return -1;
    Table *t = malloc(sizeof(Table));
    t->name    = strdup(name);
    t->columns = NULL;
    t->next    = symbol_table;
    symbol_table = t;
    return 0;
}

Column *find_column_in_table(Table *t, const char *colname)
{
    for (Column *c = t->columns; c; c = c->next)
        if (strcmp(c->name, colname) == 0) return c;
    return NULL;
}

int add_column_to_table(Table *t, const char *colname,
                        ColType type, int varchar_size)
{
    if (find_column_in_table(t, colname)) return -1;
    Column *c = malloc(sizeof(Column));
    c->name         = strdup(colname);
    c->type         = type;
    c->varchar_size = varchar_size;
    c->next         = t->columns;
    t->columns      = c;
    return 0;
}

/* ── query context ───────────────────────────────────────────────────────── */

void reset_query_ctx(void)
{
    for (int i = 0; i < query_ctx_size; i++)
        free(query_ctx[i].alias);
    query_ctx_size = 0;
}

int add_to_query_ctx(Table *t, const char *alias)
{
    if (alias) {
        for (int i = 0; i < query_ctx_size; i++)
            if (query_ctx[i].alias &&
                strcmp(query_ctx[i].alias, alias) == 0) return -1;
    }
    if (query_ctx_size >= MAX_ALIASES) return -1;
    query_ctx[query_ctx_size].table = t;
    query_ctx[query_ctx_size].alias = alias ? strdup(alias) : NULL;
    query_ctx_size++;
    return 0;
}

Table *resolve_name(const char *name)
{
    for (int i = 0; i < query_ctx_size; i++) {
        if (query_ctx[i].alias &&
            strcmp(query_ctx[i].alias, name) == 0)
            return query_ctx[i].table;
        if (!query_ctx[i].alias &&
            strcmp(query_ctx[i].table->name, name) == 0)
            return query_ctx[i].table;
    }
    return NULL;
}

int validate_column(const char *colname)
{
    for (int i = 0; i < query_ctx_size; i++)
        if (find_column_in_table(query_ctx[i].table, colname)) return 0;
    return -1;
}

int validate_qualified_column(const char *tblname, const char *colname)
{
    Table *t = resolve_name(tblname);
    if (!t) return -1;
    if (!find_column_in_table(t, colname)) return -1;
    return 0;
}

ColType column_type(const char *colname)
{
    for (int i = 0; i < query_ctx_size; i++) {
        Column *c = find_column_in_table(query_ctx[i].table, colname);
        if (c) return c->type;
    }
    return TYPE_INT;
}

ColType qualified_column_type(const char *tblname, const char *colname)
{
    Table *t = resolve_name(tblname);
    if (!t) return TYPE_INT;
    Column *c = find_column_in_table(t, colname);
    if (!c) return TYPE_INT;
    return c->type;
}

int types_compatible(ColType coltype, int lit_type)
{
    switch (coltype) {
        case TYPE_INT:     return lit_type == LIT_INT;
        case TYPE_FLOAT:   return lit_type == LIT_INT || lit_type == LIT_FLOAT;
        case TYPE_VARCHAR: return lit_type == LIT_STRING;
    }
    return 0;
}
