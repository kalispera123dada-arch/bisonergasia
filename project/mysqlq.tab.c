/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "mysqlq.y"

/*
 * mysqlq.y — Bison parser for the mySQLq pseudo-SQL language
 *
 * Covers:
 *   Q1 (60%) — full BNF grammar + syntactic analysis
 *   Q2 (15%) — semantic checks (table/column existence, type compatibility)
 *   Q3 (25%) — JOIN support + table aliases (AS)
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


#line 233 "mysqlq.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "mysqlq.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_SELECT = 3,                     /* SELECT  */
  YYSYMBOL_FROM = 4,                       /* FROM  */
  YYSYMBOL_WHERE = 5,                      /* WHERE  */
  YYSYMBOL_LIMIT = 6,                      /* LIMIT  */
  YYSYMBOL_GROUP = 7,                      /* GROUP  */
  YYSYMBOL_ORDER = 8,                      /* ORDER  */
  YYSYMBOL_BY = 9,                         /* BY  */
  YYSYMBOL_IN_KW = 10,                     /* IN_KW  */
  YYSYMBOL_AND = 11,                       /* AND  */
  YYSYMBOL_OR = 12,                        /* OR  */
  YYSYMBOL_NOT = 13,                       /* NOT  */
  YYSYMBOL_CREATE = 14,                    /* CREATE  */
  YYSYMBOL_TABLE = 15,                     /* TABLE  */
  YYSYMBOL_INT_TYPE = 16,                  /* INT_TYPE  */
  YYSYMBOL_FLOAT_TYPE = 17,                /* FLOAT_TYPE  */
  YYSYMBOL_VARCHAR = 18,                   /* VARCHAR  */
  YYSYMBOL_JOIN = 19,                      /* JOIN  */
  YYSYMBOL_ON = 20,                        /* ON  */
  YYSYMBOL_AS = 21,                        /* AS  */
  YYSYMBOL_EQ = 22,                        /* EQ  */
  YYSYMBOL_NE = 23,                        /* NE  */
  YYSYMBOL_GT = 24,                        /* GT  */
  YYSYMBOL_LT = 25,                        /* LT  */
  YYSYMBOL_GE = 26,                        /* GE  */
  YYSYMBOL_LE = 27,                        /* LE  */
  YYSYMBOL_COMMA = 28,                     /* COMMA  */
  YYSYMBOL_SEMICOLON = 29,                 /* SEMICOLON  */
  YYSYMBOL_LPAREN = 30,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 31,                    /* RPAREN  */
  YYSYMBOL_STAR = 32,                      /* STAR  */
  YYSYMBOL_DOT = 33,                       /* DOT  */
  YYSYMBOL_INT_LIT = 34,                   /* INT_LIT  */
  YYSYMBOL_FLOAT_LIT = 35,                 /* FLOAT_LIT  */
  YYSYMBOL_STRING_LIT = 36,                /* STRING_LIT  */
  YYSYMBOL_IDENTIFIER = 37,                /* IDENTIFIER  */
  YYSYMBOL_YYACCEPT = 38,                  /* $accept  */
  YYSYMBOL_program = 39,                   /* program  */
  YYSYMBOL_statement_list = 40,            /* statement_list  */
  YYSYMBOL_statement = 41,                 /* statement  */
  YYSYMBOL_create_stmt = 42,               /* create_stmt  */
  YYSYMBOL_43_1 = 43,                      /* $@1  */
  YYSYMBOL_column_list = 44,               /* column_list  */
  YYSYMBOL_column_def = 45,                /* column_def  */
  YYSYMBOL_select_stmt = 46,               /* select_stmt  */
  YYSYMBOL_47_2 = 47,                      /* $@2  */
  YYSYMBOL_48_3 = 48,                      /* $@3  */
  YYSYMBOL_49_4 = 49,                      /* $@4  */
  YYSYMBOL_col_select = 50,                /* col_select  */
  YYSYMBOL_select_col_ref_list = 51,       /* select_col_ref_list  */
  YYSYMBOL_select_col_ref = 52,            /* select_col_ref  */
  YYSYMBOL_table_ref = 53,                 /* table_ref  */
  YYSYMBOL_join_clauses = 54,              /* join_clauses  */
  YYSYMBOL_join_clause = 55,               /* join_clause  */
  YYSYMBOL_56_5 = 56,                      /* $@5  */
  YYSYMBOL_qualified_col = 57,             /* qualified_col  */
  YYSYMBOL_opt_where = 58,                 /* opt_where  */
  YYSYMBOL_condition = 59,                 /* condition  */
  YYSYMBOL_simple_cond = 60,               /* simple_cond  */
  YYSYMBOL_col_ref = 61,                   /* col_ref  */
  YYSYMBOL_lit_list = 62,                  /* lit_list  */
  YYSYMBOL_comp_op = 63,                   /* comp_op  */
  YYSYMBOL_literal = 64,                   /* literal  */
  YYSYMBOL_opt_group = 65,                 /* opt_group  */
  YYSYMBOL_opt_order = 66,                 /* opt_order  */
  YYSYMBOL_validated_col_ref_list = 67,    /* validated_col_ref_list  */
  YYSYMBOL_validated_col_ref = 68,         /* validated_col_ref  */
  YYSYMBOL_opt_limit = 69                  /* opt_limit  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  10
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   88

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  32
/* YYNRULES -- Number of rules.  */
#define YYNRULES  64
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  114

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   292


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   199,   199,   203,   204,   208,   209,   218,   217,   237,
     238,   242,   256,   270,   299,   301,   303,   299,   326,   327,
     332,   333,   337,   342,   355,   370,   398,   399,   404,   403,
     455,   462,   469,   470,   474,   475,   476,   477,   478,   482,
     516,   549,   586,   588,   600,   602,   612,   613,   614,   615,
     616,   617,   622,   623,   624,   630,   631,   637,   638,   643,
     644,   648,   658,   673,   674
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "SELECT", "FROM",
  "WHERE", "LIMIT", "GROUP", "ORDER", "BY", "IN_KW", "AND", "OR", "NOT",
  "CREATE", "TABLE", "INT_TYPE", "FLOAT_TYPE", "VARCHAR", "JOIN", "ON",
  "AS", "EQ", "NE", "GT", "LT", "GE", "LE", "COMMA", "SEMICOLON", "LPAREN",
  "RPAREN", "STAR", "DOT", "INT_LIT", "FLOAT_LIT", "STRING_LIT",
  "IDENTIFIER", "$accept", "program", "statement_list", "statement",
  "create_stmt", "$@1", "column_list", "column_def", "select_stmt", "$@2",
  "$@3", "$@4", "col_select", "select_col_ref_list", "select_col_ref",
  "table_ref", "join_clauses", "join_clause", "$@5", "qualified_col",
  "opt_where", "condition", "simple_cond", "col_ref", "lit_list",
  "comp_op", "literal", "opt_group", "opt_order", "validated_col_ref_list",
  "validated_col_ref", "opt_limit", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-75)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
       1,   -75,     6,    32,     1,   -75,    11,    13,   -27,     7,
     -75,   -75,   -75,   -75,   -75,     8,    39,    17,   -75,   -75,
       9,    10,    14,    19,   -75,    27,   -75,   -75,    15,    16,
     -75,    18,   -20,   -75,   -75,    31,   -75,   -75,    24,    15,
     -75,    10,    50,   -75,    22,   -75,   -75,   -12,    51,    26,
      40,   -12,   -12,    28,    12,   -75,     4,    53,    55,   -75,
      29,   -75,    -9,    30,   -12,   -12,    34,    49,   -75,   -75,
     -75,   -75,   -75,   -75,     3,    33,    56,    62,    36,    52,
     -75,   -75,   -75,    60,     3,    42,   -75,   -75,   -75,   -75,
      43,    45,   -75,    33,    41,   -75,    44,    29,   -19,   -75,
       3,    46,    33,    45,   -75,   -75,   -75,     3,   -75,   -15,
     -75,   -75,   -75,   -75
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    14,     0,     0,     2,     3,     0,     0,     0,     0,
       1,     4,     5,     6,    18,    22,     0,    19,    20,     7,
       0,     0,     0,     0,    23,    24,    15,    21,     0,     0,
      26,     0,     0,     9,    25,    16,    11,    12,     0,     0,
       8,     0,    32,    27,     0,    10,    28,     0,    55,     0,
       0,     0,     0,    42,    33,    34,     0,     0,    57,    13,
       0,    37,     0,     0,     0,     0,     0,     0,    46,    47,
      48,    49,    50,    51,     0,     0,     0,    63,    31,     0,
      38,    43,    35,    36,     0,     0,    52,    53,    54,    39,
      61,    56,    59,     0,     0,    17,     0,     0,     0,    44,
       0,     0,     0,    58,    64,    30,    29,     0,    40,     0,
      62,    60,    45,    41
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -75,   -75,   -75,    73,   -75,   -75,   -75,    47,   -75,   -75,
     -75,   -75,   -75,   -75,    57,    37,   -75,   -75,   -75,   -17,
     -75,   -45,   -75,   -75,   -18,   -75,   -74,   -75,   -75,    -8,
     -14,   -75
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     3,     4,     5,     6,    23,    32,    33,     7,     8,
      30,    42,    16,    17,    18,    26,    35,    43,    50,    79,
      48,    54,    55,    56,    98,    74,    99,    58,    77,    91,
      92,    95
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      89,    51,    64,    65,     1,    14,    61,    62,    39,   107,
      15,    40,   108,   107,    66,     2,   113,    67,    52,    82,
      83,     9,    80,    64,    65,    53,    68,    69,    70,    71,
      72,    73,    10,   112,    36,    37,    38,    86,    87,    88,
      12,    20,    13,    21,    19,    22,    24,    25,    29,    28,
      41,    15,    31,    34,    44,    47,    49,    59,    57,    85,
      60,    63,    75,    76,    84,    93,    78,    81,    94,    96,
      90,    64,   100,   102,    97,   104,   101,    11,    46,    27,
     106,   105,   109,   110,     0,   103,    45,     0,   111
};

static const yytype_int8 yycheck[] =
{
      74,    13,    11,    12,     3,    32,    51,    52,    28,    28,
      37,    31,    31,    28,    10,    14,    31,    13,    30,    64,
      65,    15,    31,    11,    12,    37,    22,    23,    24,    25,
      26,    27,     0,   107,    16,    17,    18,    34,    35,    36,
      29,    33,    29,     4,    37,    28,    37,    37,    21,    30,
      19,    37,    37,    37,    30,     5,    34,    31,     7,    10,
      20,    33,     9,     8,    30,     9,    37,    37,     6,    33,
      37,    11,    30,    28,    22,    34,    33,     4,    41,    22,
      97,    37,   100,    37,    -1,    93,    39,    -1,   102
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,    14,    39,    40,    41,    42,    46,    47,    15,
       0,    41,    29,    29,    32,    37,    50,    51,    52,    37,
      33,     4,    28,    43,    37,    37,    53,    52,    30,    21,
      48,    37,    44,    45,    37,    54,    16,    17,    18,    28,
      31,    19,    49,    55,    30,    45,    53,     5,    58,    34,
      56,    13,    30,    37,    59,    60,    61,     7,    65,    31,
      20,    59,    59,    33,    11,    12,    10,    13,    22,    23,
      24,    25,    26,    27,    63,     9,     8,    66,    37,    57,
      31,    37,    59,    59,    30,    10,    34,    35,    36,    64,
      37,    67,    68,     9,     6,    69,    33,    22,    62,    64,
      30,    33,    28,    67,    34,    37,    57,    28,    31,    62,
      37,    68,    64,    31
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    38,    39,    40,    40,    41,    41,    43,    42,    44,
      44,    45,    45,    45,    47,    48,    49,    46,    50,    50,
      51,    51,    52,    52,    53,    53,    54,    54,    56,    55,
      57,    57,    58,    58,    59,    59,    59,    59,    59,    60,
      60,    60,    61,    61,    62,    62,    63,    63,    63,    63,
      63,    63,    64,    64,    64,    65,    65,    66,    66,    67,
      67,    68,    68,    69,    69
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     2,     2,     0,     7,     1,
       3,     2,     2,     5,     0,     0,     0,    12,     1,     1,
       1,     3,     1,     3,     1,     3,     0,     2,     0,     7,
       3,     1,     0,     2,     1,     3,     3,     2,     3,     3,
       5,     6,     1,     3,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     3,     0,     3,     1,
       3,     1,     3,     0,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 7: /* $@1: %empty  */
#line 218 "mysqlq.y"
        {
            /* Q2a: table name must be unique */
            if (add_table((yyvsp[0].sval)) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: table '%s' already defined", (yyvsp[0].sval));
                sem_error(buf);
                YYABORT;
            }
            cur_create_table = find_table((yyvsp[0].sval));
            free((yyvsp[0].sval));
        }
#line 1373 "mysqlq.tab.c"
    break;

  case 8: /* create_stmt: CREATE TABLE IDENTIFIER $@1 LPAREN column_list RPAREN  */
#line 231 "mysqlq.y"
        {
            cur_create_table = NULL;
        }
#line 1381 "mysqlq.tab.c"
    break;

  case 11: /* column_def: IDENTIFIER INT_TYPE  */
#line 243 "mysqlq.y"
        {
            if (cur_create_table &&
                add_column_to_table(cur_create_table, (yyvsp[-1].sval), TYPE_INT, 0) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: duplicate column '%s' in table '%s'",
                         (yyvsp[-1].sval), cur_create_table->name);
                sem_error(buf);
                free((yyvsp[-1].sval));
                YYABORT;
            }
            free((yyvsp[-1].sval));
        }
#line 1399 "mysqlq.tab.c"
    break;

  case 12: /* column_def: IDENTIFIER FLOAT_TYPE  */
#line 257 "mysqlq.y"
        {
            if (cur_create_table &&
                add_column_to_table(cur_create_table, (yyvsp[-1].sval), TYPE_FLOAT, 0) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: duplicate column '%s' in table '%s'",
                         (yyvsp[-1].sval), cur_create_table->name);
                sem_error(buf);
                free((yyvsp[-1].sval));
                YYABORT;
            }
            free((yyvsp[-1].sval));
        }
#line 1417 "mysqlq.tab.c"
    break;

  case 13: /* column_def: IDENTIFIER VARCHAR LPAREN INT_LIT RPAREN  */
#line 271 "mysqlq.y"
        {
            if ((yyvsp[-1].ival) <= 0) {
                sem_error("Semantic error: VARCHAR size must be strictly positive");
                free((yyvsp[-4].sval));
                YYABORT;
            }
            if (cur_create_table &&
                add_column_to_table(cur_create_table, (yyvsp[-4].sval), TYPE_VARCHAR, (yyvsp[-1].ival)) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: duplicate column '%s' in table '%s'",
                         (yyvsp[-4].sval), cur_create_table->name);
                sem_error(buf);
                free((yyvsp[-4].sval));
                YYABORT;
            }
            free((yyvsp[-4].sval));
        }
#line 1440 "mysqlq.tab.c"
    break;

  case 14: /* $@2: %empty  */
#line 299 "mysqlq.y"
             { pending_cols_clear(); }
#line 1446 "mysqlq.tab.c"
    break;

  case 15: /* $@3: %empty  */
#line 301 "mysqlq.y"
        { free((yyvsp[0].sval)); }
#line 1452 "mysqlq.tab.c"
    break;

  case 16: /* $@4: %empty  */
#line 303 "mysqlq.y"
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
#line 1468 "mysqlq.tab.c"
    break;

  case 17: /* select_stmt: SELECT $@2 col_select FROM table_ref $@3 join_clauses $@4 opt_where opt_group opt_order opt_limit  */
#line 318 "mysqlq.y"
        {
            reset_query_ctx();
        }
#line 1476 "mysqlq.tab.c"
    break;

  case 22: /* select_col_ref: IDENTIFIER  */
#line 338 "mysqlq.y"
        {
            pending_cols_add((yyvsp[0].sval));
            free((yyvsp[0].sval));
        }
#line 1485 "mysqlq.tab.c"
    break;

  case 23: /* select_col_ref: IDENTIFIER DOT IDENTIFIER  */
#line 343 "mysqlq.y"
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s.%s", (yyvsp[-2].sval), (yyvsp[0].sval));
            pending_cols_add(buf);
            free((yyvsp[-2].sval));
            free((yyvsp[0].sval));
        }
#line 1497 "mysqlq.tab.c"
    break;

  case 24: /* table_ref: IDENTIFIER  */
#line 356 "mysqlq.y"
        {
            /* Q2b: table must have been CREATEd */
            Table *t = find_table((yyvsp[0].sval));
            if (!t) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: table '%s' not defined", (yyvsp[0].sval));
                sem_error(buf);
                free((yyvsp[0].sval));
                YYABORT;
            }
            add_to_query_ctx(t, NULL);
            (yyval.sval) = (yyvsp[0].sval);
        }
#line 1516 "mysqlq.tab.c"
    break;

  case 25: /* table_ref: IDENTIFIER AS IDENTIFIER  */
#line 371 "mysqlq.y"
        {
            /* Q3b: alias */
            Table *t = find_table((yyvsp[-2].sval));
            if (!t) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: table '%s' not defined", (yyvsp[-2].sval));
                sem_error(buf);
                free((yyvsp[-2].sval)); free((yyvsp[0].sval));
                YYABORT;
            }
            if (add_to_query_ctx(t, (yyvsp[0].sval)) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: alias '%s' already in use", (yyvsp[0].sval));
                sem_error(buf);
                free((yyvsp[-2].sval)); free((yyvsp[0].sval));
                YYABORT;
            }
            (yyval.sval) = (yyvsp[-2].sval);
            free((yyvsp[0].sval));
        }
#line 1543 "mysqlq.tab.c"
    break;

  case 28: /* $@5: %empty  */
#line 404 "mysqlq.y"
        { free((yyvsp[0].sval)); }
#line 1549 "mysqlq.tab.c"
    break;

  case 29: /* join_clause: JOIN table_ref $@5 ON qualified_col EQ qualified_col  */
#line 406 "mysqlq.y"
        {
            char *left  = (yyvsp[-2].sval);
            char *right = (yyvsp[0].sval);

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
#line 1600 "mysqlq.tab.c"
    break;

  case 30: /* qualified_col: IDENTIFIER DOT IDENTIFIER  */
#line 456 "mysqlq.y"
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s.%s", (yyvsp[-2].sval), (yyvsp[0].sval));
            (yyval.sval) = strdup(buf);
            free((yyvsp[-2].sval)); free((yyvsp[0].sval));
        }
#line 1611 "mysqlq.tab.c"
    break;

  case 31: /* qualified_col: IDENTIFIER  */
#line 463 "mysqlq.y"
        { (yyval.sval) = (yyvsp[0].sval); }
#line 1617 "mysqlq.tab.c"
    break;

  case 39: /* simple_cond: col_ref comp_op literal  */
#line 483 "mysqlq.y"
        {
            /* Q2e: type compatibility check */
            char *cr  = (yyvsp[-2].sval);
            int   lt  = (yyvsp[0].lit_type);
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
#line 1655 "mysqlq.tab.c"
    break;

  case 40: /* simple_cond: col_ref IN_KW LPAREN lit_list RPAREN  */
#line 517 "mysqlq.y"
        {
            char *cr  = (yyvsp[-4].sval);
            int   lt  = (yyvsp[-1].lit_type);
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
#line 1692 "mysqlq.tab.c"
    break;

  case 41: /* simple_cond: col_ref NOT IN_KW LPAREN lit_list RPAREN  */
#line 550 "mysqlq.y"
        {
            char *cr  = (yyvsp[-5].sval);
            int   lt  = (yyvsp[-1].lit_type);
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
#line 1729 "mysqlq.tab.c"
    break;

  case 42: /* col_ref: IDENTIFIER  */
#line 587 "mysqlq.y"
        { (yyval.sval) = (yyvsp[0].sval); }
#line 1735 "mysqlq.tab.c"
    break;

  case 43: /* col_ref: IDENTIFIER DOT IDENTIFIER  */
#line 589 "mysqlq.y"
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s.%s", (yyvsp[-2].sval), (yyvsp[0].sval));
            (yyval.sval) = strdup(buf);
            free((yyvsp[-2].sval)); free((yyvsp[0].sval));
        }
#line 1746 "mysqlq.tab.c"
    break;

  case 44: /* lit_list: literal  */
#line 601 "mysqlq.y"
        { (yyval.lit_type) = (yyvsp[0].lit_type); }
#line 1752 "mysqlq.tab.c"
    break;

  case 45: /* lit_list: lit_list COMMA literal  */
#line 603 "mysqlq.y"
        {
            /* if types differ, mark as mixed (-1) */
            (yyval.lit_type) = ((yyvsp[-2].lit_type) == (yyvsp[0].lit_type)) ? (yyvsp[-2].lit_type) : -1;
        }
#line 1761 "mysqlq.tab.c"
    break;

  case 46: /* comp_op: EQ  */
#line 612 "mysqlq.y"
          { (yyval.ival) = 0; }
#line 1767 "mysqlq.tab.c"
    break;

  case 47: /* comp_op: NE  */
#line 613 "mysqlq.y"
          { (yyval.ival) = 1; }
#line 1773 "mysqlq.tab.c"
    break;

  case 48: /* comp_op: GT  */
#line 614 "mysqlq.y"
          { (yyval.ival) = 2; }
#line 1779 "mysqlq.tab.c"
    break;

  case 49: /* comp_op: LT  */
#line 615 "mysqlq.y"
          { (yyval.ival) = 3; }
#line 1785 "mysqlq.tab.c"
    break;

  case 50: /* comp_op: GE  */
#line 616 "mysqlq.y"
          { (yyval.ival) = 4; }
#line 1791 "mysqlq.tab.c"
    break;

  case 51: /* comp_op: LE  */
#line 617 "mysqlq.y"
          { (yyval.ival) = 5; }
#line 1797 "mysqlq.tab.c"
    break;

  case 52: /* literal: INT_LIT  */
#line 622 "mysqlq.y"
                 { (yyval.lit_type) = LIT_INT;    }
#line 1803 "mysqlq.tab.c"
    break;

  case 53: /* literal: FLOAT_LIT  */
#line 623 "mysqlq.y"
                 { (yyval.lit_type) = LIT_FLOAT;  }
#line 1809 "mysqlq.tab.c"
    break;

  case 54: /* literal: STRING_LIT  */
#line 624 "mysqlq.y"
                 { free((yyvsp[0].sval)); (yyval.lit_type) = LIT_STRING; }
#line 1815 "mysqlq.tab.c"
    break;

  case 61: /* validated_col_ref: IDENTIFIER  */
#line 649 "mysqlq.y"
        {
            if (validate_column((yyvsp[0].sval)) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: column '%s' not found in query tables", (yyvsp[0].sval));
                sem_error(buf); free((yyvsp[0].sval)); YYABORT;
            }
            free((yyvsp[0].sval));
        }
#line 1829 "mysqlq.tab.c"
    break;

  case 62: /* validated_col_ref: IDENTIFIER DOT IDENTIFIER  */
#line 659 "mysqlq.y"
        {
            if (validate_qualified_column((yyvsp[-2].sval), (yyvsp[0].sval)) != 0) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Semantic error: column '%s.%s' not found", (yyvsp[-2].sval), (yyvsp[0].sval));
                sem_error(buf); free((yyvsp[-2].sval)); free((yyvsp[0].sval)); YYABORT;
            }
            free((yyvsp[-2].sval)); free((yyvsp[0].sval));
        }
#line 1843 "mysqlq.tab.c"
    break;

  case 64: /* opt_limit: LIMIT INT_LIT  */
#line 675 "mysqlq.y"
        {
            if ((yyvsp[0].ival) <= 0) {
                sem_error("Semantic error: LIMIT value must be strictly positive");
                YYABORT;
            }
        }
#line 1854 "mysqlq.tab.c"
    break;


#line 1858 "mysqlq.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 683 "mysqlq.y"


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
