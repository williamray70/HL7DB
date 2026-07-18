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
#line 7 "src/query/hl7sql.y"

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


#line 92 "src/query/hl7sql.tab.c"

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

#include "hl7sql.tab.h"
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
  YYSYMBOL_INSERT = 6,                     /* INSERT  */
  YYSYMBOL_MESSAGE = 7,                    /* MESSAGE  */
  YYSYMBOL_AND = 8,                        /* AND  */
  YYSYMBOL_OR = 9,                         /* OR  */
  YYSYMBOL_LIKE = 10,                      /* LIKE  */
  YYSYMBOL_BETWEEN = 11,                   /* BETWEEN  */
  YYSYMBOL_IN = 12,                        /* IN  */
  YYSYMBOL_COUNT = 13,                     /* COUNT  */
  YYSYMBOL_GROUP = 14,                     /* GROUP  */
  YYSYMBOL_BY = 15,                        /* BY  */
  YYSYMBOL_AS = 16,                        /* AS  */
  YYSYMBOL_CREATE = 17,                    /* CREATE  */
  YYSYMBOL_DROP = 18,                      /* DROP  */
  YYSYMBOL_QUEUE = 19,                     /* QUEUE  */
  YYSYMBOL_INTO = 20,                      /* INTO  */
  YYSYMBOL_ALL = 21,                       /* ALL  */
  YYSYMBOL_POP = 22,                       /* POP  */
  YYSYMBOL_CLEAR = 23,                     /* CLEAR  */
  YYSYMBOL_HISTORY = 24,                   /* HISTORY  */
  YYSYMBOL_SEGMENT = 25,                   /* SEGMENT  */
  YYSYMBOL_TIMESTAMP = 26,                 /* TIMESTAMP  */
  YYSYMBOL_LAST_N_HOURS = 27,              /* LAST_N_HOURS  */
  YYSYMBOL_LAST_N_DAYS = 28,               /* LAST_N_DAYS  */
  YYSYMBOL_EQ = 29,                        /* EQ  */
  YYSYMBOL_NE = 30,                        /* NE  */
  YYSYMBOL_LT = 31,                        /* LT  */
  YYSYMBOL_LE = 32,                        /* LE  */
  YYSYMBOL_GT = 33,                        /* GT  */
  YYSYMBOL_GE = 34,                        /* GE  */
  YYSYMBOL_LPAREN = 35,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 36,                    /* RPAREN  */
  YYSYMBOL_LBRACKET = 37,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 38,                  /* RBRACKET  */
  YYSYMBOL_COMMA = 39,                     /* COMMA  */
  YYSYMBOL_DOT = 40,                       /* DOT  */
  YYSYMBOL_STAR = 41,                      /* STAR  */
  YYSYMBOL_SEMICOLON = 42,                 /* SEMICOLON  */
  YYSYMBOL_ERROR_TOKEN = 43,               /* ERROR_TOKEN  */
  YYSYMBOL_STRING_LITERAL = 44,            /* STRING_LITERAL  */
  YYSYMBOL_IDENTIFIER = 45,                /* IDENTIFIER  */
  YYSYMBOL_NUMBER = 46,                    /* NUMBER  */
  YYSYMBOL_YYACCEPT = 47,                  /* $accept  */
  YYSYMBOL_statement = 48,                 /* statement  */
  YYSYMBOL_select_stmt = 49,               /* select_stmt  */
  YYSYMBOL_column_list = 50,               /* column_list  */
  YYSYMBOL_column_spec = 51,               /* column_spec  */
  YYSYMBOL_expression = 52,                /* expression  */
  YYSYMBOL_aggregate_func = 53,            /* aggregate_func  */
  YYSYMBOL_time_function = 54,             /* time_function  */
  YYSYMBOL_field_ref = 55,                 /* field_ref  */
  YYSYMBOL_where_clause = 56,              /* where_clause  */
  YYSYMBOL_condition = 57,                 /* condition  */
  YYSYMBOL_comparison = 58,                /* comparison  */
  YYSYMBOL_insert_stmt = 59,               /* insert_stmt  */
  YYSYMBOL_create_queue_stmt = 60,         /* create_queue_stmt  */
  YYSYMBOL_drop_queue_stmt = 61,           /* drop_queue_stmt  */
  YYSYMBOL_pop_message_stmt = 62,          /* pop_message_stmt  */
  YYSYMBOL_clear_history_stmt = 63,        /* clear_history_stmt  */
  YYSYMBOL_queue_ref = 64                  /* queue_ref  */
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
#define YYFINAL  33
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   112

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  47
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  18
/* YYNRULES -- Number of rules.  */
#define YYNRULES  53
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  105

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   301


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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    70,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    86,    93,    96,   100,   117,   120,
     128,   129,   130,   131,   132,   136,   141,   144,   151,   154,
     161,   169,   170,   175,   176,   177,   178,   183,   184,   185,
     186,   187,   188,   189,   190,   197,   206,   214,   222,   230,
     238,   242,   246,   249
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
  "WHERE", "INSERT", "MESSAGE", "AND", "OR", "LIKE", "BETWEEN", "IN",
  "COUNT", "GROUP", "BY", "AS", "CREATE", "DROP", "QUEUE", "INTO", "ALL",
  "POP", "CLEAR", "HISTORY", "SEGMENT", "TIMESTAMP", "LAST_N_HOURS",
  "LAST_N_DAYS", "EQ", "NE", "LT", "LE", "GT", "GE", "LPAREN", "RPAREN",
  "LBRACKET", "RBRACKET", "COMMA", "DOT", "STAR", "SEMICOLON",
  "ERROR_TOKEN", "STRING_LITERAL", "IDENTIFIER", "NUMBER", "$accept",
  "statement", "select_stmt", "column_list", "column_spec", "expression",
  "aggregate_func", "time_function", "field_ref", "where_clause",
  "condition", "comparison", "insert_stmt", "create_queue_stmt",
  "drop_queue_stmt", "pop_message_stmt", "clear_history_stmt", "queue_ref", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-81)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      86,   -11,     1,    -7,    -6,    11,     2,    19,   -22,   -15,
     -14,   -10,     3,    15,     6,    18,   -81,     8,    24,   -81,
     -81,   -81,    -3,   -81,    45,   -81,   -81,   -81,    40,    17,
      23,    59,    27,   -81,   -81,   -81,   -81,   -81,   -81,   -81,
      -4,    34,    20,    47,   -16,    25,    35,    46,   -81,   -81,
      49,   -81,    28,    54,    33,    60,    61,   -81,   -81,    55,
      93,   -81,   -81,    56,   -81,   -81,   -81,    53,   -81,   -81,
      77,    21,   -81,   -81,    67,   -81,    21,    44,    -2,   -81,
     -81,    -5,    25,    25,    25,    25,    25,    25,    25,    25,
      25,    25,   -81,   -81,    98,   -81,   -81,   -81,   -81,   -81,
     -81,   -81,   -81,    25,   -81
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     3,     5,
       7,     9,    11,    13,     0,     0,    23,     0,     0,    15,
      24,    25,     0,    16,    18,    21,    22,    20,     0,     0,
       0,     0,     0,     1,     2,     4,     6,     8,    10,    12,
       0,     0,     0,     0,     0,     0,     0,     0,    46,    47,
       0,    49,     0,     0,     0,     0,     0,    53,    52,    50,
      31,    17,    19,     0,    48,    26,    27,     0,    28,    29,
       0,     0,    14,    45,     0,    51,     0,     0,    32,    33,
      30,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    36,    43,     0,    37,    38,    39,    40,    41,
      42,    34,    35,     0,    44
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -81,   -81,   -81,   -81,    62,    -1,   -81,   -81,   -81,   -81,
      36,   -80,   -81,   -81,   -81,   -81,   -81,   -81
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     7,     8,    22,    23,    77,    25,    26,    27,    72,
      78,    79,     9,    10,    11,    12,    13,    60
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      24,    44,    14,    90,    91,    57,    90,    91,    28,    14,
     101,   102,    29,    30,    15,    16,    17,    18,    31,    33,
      34,    15,    16,    17,    18,    58,    32,    35,    36,    59,
      19,    92,    37,    20,    14,    21,    45,    52,    14,    53,
      20,    40,    21,    42,    24,    38,    15,    16,    17,    18,
      15,    16,    17,    18,    82,    83,    76,    39,    41,    43,
      47,    46,    48,    50,    65,    20,    55,    21,    49,    20,
      67,    21,    51,    84,    85,    86,    87,    88,    89,    54,
      62,    93,    94,    95,    96,    97,    98,    99,   100,     1,
      66,    63,     2,    56,    64,    70,    68,    69,    71,    74,
      73,    75,   104,     3,     4,    80,   103,    61,     5,     6,
       0,     0,    81
};

static const yytype_int8 yycheck[] =
{
       1,     4,    13,     8,     9,    21,     8,     9,     7,    13,
      90,    91,    19,    19,    25,    26,    27,    28,     7,     0,
      42,    25,    26,    27,    28,    41,    24,    42,    42,    45,
      41,    36,    42,    44,    13,    46,    39,    41,    13,    40,
      44,    35,    46,    35,    45,    42,    25,    26,    27,    28,
      25,    26,    27,    28,    10,    11,    35,    42,    40,    35,
      20,    16,    45,     4,    36,    44,    46,    46,    45,    44,
      37,    46,    45,    29,    30,    31,    32,    33,    34,    45,
      45,    82,    83,    84,    85,    86,    87,    88,    89,     3,
      36,    45,     6,    46,    45,    40,    36,    36,     5,    46,
      44,    24,   103,    17,    18,    38,     8,    45,    22,    23,
      -1,    -1,    76
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     6,    17,    18,    22,    23,    48,    49,    59,
      60,    61,    62,    63,    13,    25,    26,    27,    28,    41,
      44,    46,    50,    51,    52,    53,    54,    55,     7,    19,
      19,     7,    24,     0,    42,    42,    42,    42,    42,    42,
      35,    40,    35,    35,     4,    39,    16,    20,    45,    45,
       4,    45,    41,    52,    45,    46,    46,    21,    41,    45,
      64,    51,    45,    45,    45,    36,    36,    37,    36,    36,
      40,     5,    56,    44,    46,    24,    35,    52,    57,    58,
      38,    57,    10,    11,    29,    30,    31,    32,    33,    34,
       8,     9,    36,    52,    52,    52,    52,    52,    52,    52,
      52,    58,    58,     8,    52
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    47,    48,    48,    48,    48,    48,    48,    48,    48,
      48,    48,    48,    48,    49,    50,    50,    50,    51,    51,
      52,    52,    52,    52,    52,    52,    53,    53,    54,    54,
      55,    56,    56,    57,    57,    57,    57,    58,    58,    58,
      58,    58,    58,    58,    58,    59,    60,    61,    62,    63,
      64,    64,    64,    64
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     2,     1,     2,     1,     2,     1,
       2,     1,     2,     1,     5,     1,     1,     3,     1,     3,
       1,     1,     1,     1,     1,     1,     4,     4,     4,     4,
       6,     0,     2,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     5,     5,     3,     3,     4,     3,
       1,     3,     1,     1
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
        yyerror (result, YY_("syntax error: cannot back up")); \
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
                  Kind, Value, result); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, ast_node_t **result)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (result);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, ast_node_t **result)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, result);
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
                 int yyrule, ast_node_t **result)
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
                       &yyvsp[(yyi + 1) - (yynrhs)], result);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, result); \
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
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, ast_node_t **result)
{
  YY_USE (yyvaluep);
  YY_USE (result);
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
yyparse (ast_node_t **result)
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
  case 2: /* statement: select_stmt SEMICOLON  */
#line 70 "src/query/hl7sql.y"
                                    { *result = (yyvsp[-1].node); }
#line 1214 "src/query/hl7sql.tab.c"
    break;

  case 3: /* statement: select_stmt  */
#line 71 "src/query/hl7sql.y"
                                    { *result = (yyvsp[0].node); }
#line 1220 "src/query/hl7sql.tab.c"
    break;

  case 4: /* statement: insert_stmt SEMICOLON  */
#line 72 "src/query/hl7sql.y"
                                    { *result = (yyvsp[-1].node); }
#line 1226 "src/query/hl7sql.tab.c"
    break;

  case 5: /* statement: insert_stmt  */
#line 73 "src/query/hl7sql.y"
                                    { *result = (yyvsp[0].node); }
#line 1232 "src/query/hl7sql.tab.c"
    break;

  case 6: /* statement: create_queue_stmt SEMICOLON  */
#line 74 "src/query/hl7sql.y"
                                  { *result = (yyvsp[-1].node); }
#line 1238 "src/query/hl7sql.tab.c"
    break;

  case 7: /* statement: create_queue_stmt  */
#line 75 "src/query/hl7sql.y"
                                  { *result = (yyvsp[0].node); }
#line 1244 "src/query/hl7sql.tab.c"
    break;

  case 8: /* statement: drop_queue_stmt SEMICOLON  */
#line 76 "src/query/hl7sql.y"
                                  { *result = (yyvsp[-1].node); }
#line 1250 "src/query/hl7sql.tab.c"
    break;

  case 9: /* statement: drop_queue_stmt  */
#line 77 "src/query/hl7sql.y"
                                  { *result = (yyvsp[0].node); }
#line 1256 "src/query/hl7sql.tab.c"
    break;

  case 10: /* statement: pop_message_stmt SEMICOLON  */
#line 78 "src/query/hl7sql.y"
                                    { *result = (yyvsp[-1].node); }
#line 1262 "src/query/hl7sql.tab.c"
    break;

  case 11: /* statement: pop_message_stmt  */
#line 79 "src/query/hl7sql.y"
                                    { *result = (yyvsp[0].node); }
#line 1268 "src/query/hl7sql.tab.c"
    break;

  case 12: /* statement: clear_history_stmt SEMICOLON  */
#line 80 "src/query/hl7sql.y"
                                    { *result = (yyvsp[-1].node); }
#line 1274 "src/query/hl7sql.tab.c"
    break;

  case 13: /* statement: clear_history_stmt  */
#line 81 "src/query/hl7sql.y"
                                    { *result = (yyvsp[0].node); }
#line 1280 "src/query/hl7sql.tab.c"
    break;

  case 14: /* select_stmt: SELECT column_list FROM queue_ref where_clause  */
#line 86 "src/query/hl7sql.y"
                                                   {
        (yyval.node) = ast_create_select((yyvsp[-3].node), (yyvsp[-1].node), (yyvsp[0].node), NULL);
    }
#line 1288 "src/query/hl7sql.tab.c"
    break;

  case 15: /* column_list: STAR  */
#line 93 "src/query/hl7sql.y"
         {
        (yyval.node) = ast_create_wildcard();
    }
#line 1296 "src/query/hl7sql.tab.c"
    break;

  case 16: /* column_list: column_spec  */
#line 96 "src/query/hl7sql.y"
                {
        ast_node_t *cols[1] = { (yyvsp[0].node) };
        (yyval.node) = ast_create_column_list(cols, 1);
    }
#line 1305 "src/query/hl7sql.tab.c"
    break;

  case 17: /* column_list: column_list COMMA column_spec  */
#line 100 "src/query/hl7sql.y"
                                  {
        /* Append to existing column list */
        ast_column_list_t *list = &((yyvsp[-2].node)->column_list);
        size_t new_size = list->num_columns + 1;
        ast_node_t **new_cols = realloc(list->columns,
                                         new_size * sizeof(ast_node_t *));
        if (new_cols) {
            new_cols[new_size - 1] = (yyvsp[0].node);
            list->columns = new_cols;
            list->num_columns = new_size;
        }
        (yyval.node) = (yyvsp[-2].node);
    }
#line 1323 "src/query/hl7sql.tab.c"
    break;

  case 18: /* column_spec: expression  */
#line 117 "src/query/hl7sql.y"
               {
        (yyval.node) = ast_create_column((yyvsp[0].node), NULL);
    }
#line 1331 "src/query/hl7sql.tab.c"
    break;

  case 19: /* column_spec: expression AS IDENTIFIER  */
#line 120 "src/query/hl7sql.y"
                             {
        (yyval.node) = ast_create_column((yyvsp[-2].node), (yyvsp[0].string));
        free((yyvsp[0].string));
    }
#line 1340 "src/query/hl7sql.tab.c"
    break;

  case 20: /* expression: field_ref  */
#line 128 "src/query/hl7sql.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 1346 "src/query/hl7sql.tab.c"
    break;

  case 21: /* expression: aggregate_func  */
#line 129 "src/query/hl7sql.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 1352 "src/query/hl7sql.tab.c"
    break;

  case 22: /* expression: time_function  */
#line 130 "src/query/hl7sql.y"
                            { (yyval.node) = (yyvsp[0].node); }
#line 1358 "src/query/hl7sql.tab.c"
    break;

  case 23: /* expression: TIMESTAMP  */
#line 131 "src/query/hl7sql.y"
                            { (yyval.node) = ast_create_timestamp_ref(); }
#line 1364 "src/query/hl7sql.tab.c"
    break;

  case 24: /* expression: STRING_LITERAL  */
#line 132 "src/query/hl7sql.y"
                            {
        (yyval.node) = ast_create_literal_string((yyvsp[0].string));
        free((yyvsp[0].string));
    }
#line 1373 "src/query/hl7sql.tab.c"
    break;

  case 25: /* expression: NUMBER  */
#line 136 "src/query/hl7sql.y"
                            { (yyval.node) = ast_create_literal_number((yyvsp[0].number)); }
#line 1379 "src/query/hl7sql.tab.c"
    break;

  case 26: /* aggregate_func: COUNT LPAREN STAR RPAREN  */
#line 141 "src/query/hl7sql.y"
                             {
        (yyval.node) = ast_create_count(NULL);  /* COUNT(*) */
    }
#line 1387 "src/query/hl7sql.tab.c"
    break;

  case 27: /* aggregate_func: COUNT LPAREN expression RPAREN  */
#line 144 "src/query/hl7sql.y"
                                   {
        (yyval.node) = ast_create_count((yyvsp[-1].node));
    }
#line 1395 "src/query/hl7sql.tab.c"
    break;

  case 28: /* time_function: LAST_N_HOURS LPAREN NUMBER RPAREN  */
#line 151 "src/query/hl7sql.y"
                                      {
        (yyval.node) = ast_create_time_function(TIME_FUNC_LAST_N_HOURS, (int)(yyvsp[-1].number));
    }
#line 1403 "src/query/hl7sql.tab.c"
    break;

  case 29: /* time_function: LAST_N_DAYS LPAREN NUMBER RPAREN  */
#line 154 "src/query/hl7sql.y"
                                     {
        (yyval.node) = ast_create_time_function(TIME_FUNC_LAST_N_DAYS, (int)(yyvsp[-1].number));
    }
#line 1411 "src/query/hl7sql.tab.c"
    break;

  case 30: /* field_ref: SEGMENT DOT IDENTIFIER LBRACKET NUMBER RBRACKET  */
#line 161 "src/query/hl7sql.y"
                                                    {
        (yyval.node) = ast_create_field_ref((yyvsp[-3].string), (int)(yyvsp[-1].number));
        free((yyvsp[-3].string));
    }
#line 1420 "src/query/hl7sql.tab.c"
    break;

  case 31: /* where_clause: %empty  */
#line 169 "src/query/hl7sql.y"
                                { (yyval.node) = NULL; }
#line 1426 "src/query/hl7sql.tab.c"
    break;

  case 32: /* where_clause: WHERE condition  */
#line 170 "src/query/hl7sql.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 1432 "src/query/hl7sql.tab.c"
    break;

  case 33: /* condition: comparison  */
#line 175 "src/query/hl7sql.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 1438 "src/query/hl7sql.tab.c"
    break;

  case 34: /* condition: condition AND comparison  */
#line 176 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_AND, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1444 "src/query/hl7sql.tab.c"
    break;

  case 35: /* condition: condition OR comparison  */
#line 177 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_OR, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1450 "src/query/hl7sql.tab.c"
    break;

  case 36: /* condition: LPAREN condition RPAREN  */
#line 178 "src/query/hl7sql.y"
                                { (yyval.node) = (yyvsp[-1].node); }
#line 1456 "src/query/hl7sql.tab.c"
    break;

  case 37: /* comparison: expression EQ expression  */
#line 183 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_EQ, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1462 "src/query/hl7sql.tab.c"
    break;

  case 38: /* comparison: expression NE expression  */
#line 184 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_NE, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1468 "src/query/hl7sql.tab.c"
    break;

  case 39: /* comparison: expression LT expression  */
#line 185 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_LT, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1474 "src/query/hl7sql.tab.c"
    break;

  case 40: /* comparison: expression LE expression  */
#line 186 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_LE, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1480 "src/query/hl7sql.tab.c"
    break;

  case 41: /* comparison: expression GT expression  */
#line 187 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_GT, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1486 "src/query/hl7sql.tab.c"
    break;

  case 42: /* comparison: expression GE expression  */
#line 188 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_GE, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1492 "src/query/hl7sql.tab.c"
    break;

  case 43: /* comparison: expression LIKE expression  */
#line 189 "src/query/hl7sql.y"
                                { (yyval.node) = ast_create_binary_op(OP_LIKE, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1498 "src/query/hl7sql.tab.c"
    break;

  case 44: /* comparison: expression BETWEEN expression AND expression  */
#line 190 "src/query/hl7sql.y"
                                                 {
        (yyval.node) = ast_create_between((yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node));
    }
#line 1506 "src/query/hl7sql.tab.c"
    break;

  case 45: /* insert_stmt: INSERT MESSAGE INTO IDENTIFIER STRING_LITERAL  */
#line 197 "src/query/hl7sql.y"
                                                  {
        (yyval.node) = ast_create_insert((yyvsp[-1].string), (yyvsp[0].string));
        free((yyvsp[-1].string));
        free((yyvsp[0].string));
    }
#line 1516 "src/query/hl7sql.tab.c"
    break;

  case 46: /* create_queue_stmt: CREATE QUEUE IDENTIFIER  */
#line 206 "src/query/hl7sql.y"
                            {
        (yyval.node) = ast_create_create_queue((yyvsp[0].string));
        free((yyvsp[0].string));
    }
#line 1525 "src/query/hl7sql.tab.c"
    break;

  case 47: /* drop_queue_stmt: DROP QUEUE IDENTIFIER  */
#line 214 "src/query/hl7sql.y"
                          {
        (yyval.node) = ast_create_drop_queue((yyvsp[0].string));
        free((yyvsp[0].string));
    }
#line 1534 "src/query/hl7sql.tab.c"
    break;

  case 48: /* pop_message_stmt: POP MESSAGE FROM IDENTIFIER  */
#line 222 "src/query/hl7sql.y"
                                {
        (yyval.node) = ast_create_pop_message((yyvsp[0].string));
        free((yyvsp[0].string));
    }
#line 1543 "src/query/hl7sql.tab.c"
    break;

  case 49: /* clear_history_stmt: CLEAR HISTORY IDENTIFIER  */
#line 230 "src/query/hl7sql.y"
                             {
        (yyval.node) = ast_create_clear_history((yyvsp[0].string));
        free((yyvsp[0].string));
    }
#line 1552 "src/query/hl7sql.tab.c"
    break;

  case 50: /* queue_ref: IDENTIFIER  */
#line 238 "src/query/hl7sql.y"
               {
        (yyval.node) = ast_create_queue_ref_named((yyvsp[0].string));
        free((yyvsp[0].string));
    }
#line 1561 "src/query/hl7sql.tab.c"
    break;

  case 51: /* queue_ref: IDENTIFIER DOT HISTORY  */
#line 242 "src/query/hl7sql.y"
                           {
        (yyval.node) = ast_create_queue_ref_history((yyvsp[-2].string));
        free((yyvsp[-2].string));
    }
#line 1570 "src/query/hl7sql.tab.c"
    break;

  case 52: /* queue_ref: STAR  */
#line 246 "src/query/hl7sql.y"
         {
        (yyval.node) = ast_create_queue_ref_wildcard();
    }
#line 1578 "src/query/hl7sql.tab.c"
    break;

  case 53: /* queue_ref: ALL  */
#line 249 "src/query/hl7sql.y"
        {
        (yyval.node) = ast_create_queue_ref_all();
    }
#line 1586 "src/query/hl7sql.tab.c"
    break;


#line 1590 "src/query/hl7sql.tab.c"

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
      yyerror (result, YY_("syntax error"));
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
                      yytoken, &yylval, result);
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, result);
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
  yyerror (result, YY_("memory exhausted"));
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
                  yytoken, &yylval, result);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, result);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 254 "src/query/hl7sql.y"


/* Additional C code */
