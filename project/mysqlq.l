%{
/*
 * mysqlq.l — Flex lexer for the mySQLq pseudo-SQL language
 *
 * Keywords are case-insensitive.
 * Identifiers are case-sensitive.
 * Supports single-line (--) and multi-line (/* *\/) comments.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mysqlq.tab.h"

void yyerror(const char *s);
%}

%option yylineno
%option noyywrap

DIGIT      [0-9]
ID_START   [a-zA-Z_]
ID_CONT    [a-zA-Z0-9_]

%%

"--"[^\n]*              { /* single-line comment — skip */ }

"/*"                    {
    /* multi-line comment */
    int c, prev = 0;
    while ((c = input()) != EOF) {
        if (prev == '*' && c == '/') break;
        prev = c;
    }
    if (c == EOF)
        yyerror("Unterminated block comment");
}

[ \t\r\n]+              { /* whitespace — skip */ }

[Ss][Ee][Ll][Ee][Cc][Tt]           { return SELECT;      }
[Ff][Rr][Oo][Mm]                   { return FROM;        }
[Ww][Hh][Ee][Rr][Ee]               { return WHERE;       }
[Ll][Ii][Mm][Ii][Tt]               { return LIMIT;       }
[Gg][Rr][Oo][Uu][Pp]               { return GROUP;       }
[Oo][Rr][Dd][Ee][Rr]               { return ORDER;       }
[Bb][Yy]                           { return BY;          }
[Aa][Nn][Dd]                       { return AND;         }
[Oo][Rr]                           { return OR;          }
[Nn][Oo][Tt]                       { return NOT;         }
[Cc][Rr][Ee][Aa][Tt][Ee]           { return CREATE;      }
[Tt][Aa][Bb][Ll][Ee]               { return TABLE;       }
[Jj][Oo][Ii][Nn]                   { return JOIN;        }
[Oo][Nn]                           { return ON;          }
[Aa][Ss]                           { return AS;          }

[Ii][Nn][Tt]                       { return INT_TYPE;    }
[Ff][Ll][Oo][Aa][Tt]               { return FLOAT_TYPE;  }
[Vv][Aa][Rr][Cc][Hh][Aa][Rr]       { return VARCHAR;     }

[Ii][Nn]                           { return IN_KW;       }

{ID_START}{ID_CONT}*    {
    yylval.sval = strdup(yytext);
    return IDENTIFIER;
}

"-"?{DIGIT}+"."{DIGIT}+ {
    yylval.dval = atof(yytext);
    return FLOAT_LIT;
}

"-"?{DIGIT}+            {
    yylval.ival = atoi(yytext);
    return INT_LIT;
}

\'[^\']*\'              {
    yylval.sval = strdup(yytext);
    return STRING_LIT;
}

"!="    { return NE; }
">="    { return GE; }
"<="    { return LE; }
">"     { return GT; }
"<"     { return LT; }
"="     { return EQ; }
"."     { return DOT; }
","     { return COMMA; }
";"     { return SEMICOLON; }
"("     { return LPAREN; }
")"     { return RPAREN; }
"*"     { return STAR; }

.       {
    char msg[64];
    snprintf(msg, sizeof(msg), "Unknown character: '%s'", yytext);
    yyerror(msg);
}

%%
