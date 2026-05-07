#ifndef YY_YY_MYSQLQ_TAB_H_INCLUDED
# define YY_YY_MYSQLQ_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    SELECT = 258,                  /* SELECT  */
    FROM = 259,                    /* FROM  */
    WHERE = 260,                   /* WHERE  */
    LIMIT = 261,                   /* LIMIT  */
    GROUP = 262,                   /* GROUP  */
    ORDER = 263,                   /* ORDER  */
    BY = 264,                      /* BY  */
    IN_KW = 265,                   /* IN_KW  */
    AND = 266,                     /* AND  */
    OR = 267,                      /* OR  */
    NOT = 268,                     /* NOT  */
    CREATE = 269,                  /* CREATE  */
    TABLE = 270,                   /* TABLE  */
    INT_TYPE = 271,                /* INT_TYPE  */
    FLOAT_TYPE = 272,              /* FLOAT_TYPE  */
    VARCHAR = 273,                 /* VARCHAR  */
    JOIN = 274,                    /* JOIN  */
    ON = 275,                      /* ON  */
    AS = 276,                      /* AS  */
    EQ = 277,                      /* EQ  */
    NE = 278,                      /* NE  */
    GT = 279,                      /* GT  */
    LT = 280,                      /* LT  */
    GE = 281,                      /* GE  */
    LE = 282,                      /* LE  */
    COMMA = 283,                   /* COMMA  */
    SEMICOLON = 284,               /* SEMICOLON  */
    LPAREN = 285,                  /* LPAREN  */
    RPAREN = 286,                  /* RPAREN  */
    STAR = 287,                    /* STAR  */
    DOT = 288,                     /* DOT  */
    INT_LIT = 289,                 /* INT_LIT  */
    FLOAT_LIT = 290,               /* FLOAT_LIT  */
    STRING_LIT = 291,              /* STRING_LIT  */
    IDENTIFIER = 292               /* IDENTIFIER  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 164 "mysqlq.y"

    int    ival;
    double dval;
    char  *sval;
    int    lit_type;

#line 108 "mysqlq.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_MYSQLQ_TAB_H_INCLUDED  */
