/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_SRC_QUERY_HL7SQL_TAB_H_INCLUDED
# define YY_YY_SRC_QUERY_HL7SQL_TAB_H_INCLUDED
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
    INSERT = 261,                  /* INSERT  */
    MESSAGE = 262,                 /* MESSAGE  */
    AND = 263,                     /* AND  */
    OR = 264,                      /* OR  */
    LIKE = 265,                    /* LIKE  */
    BETWEEN = 266,                 /* BETWEEN  */
    IN = 267,                      /* IN  */
    COUNT = 268,                   /* COUNT  */
    GROUP = 269,                   /* GROUP  */
    BY = 270,                      /* BY  */
    AS = 271,                      /* AS  */
    CREATE = 272,                  /* CREATE  */
    DROP = 273,                    /* DROP  */
    QUEUE = 274,                   /* QUEUE  */
    INTO = 275,                    /* INTO  */
    ALL = 276,                     /* ALL  */
    POP = 277,                     /* POP  */
    CLEAR = 278,                   /* CLEAR  */
    HISTORY = 279,                 /* HISTORY  */
    SEGMENT = 280,                 /* SEGMENT  */
    TIMESTAMP = 281,               /* TIMESTAMP  */
    LAST_N_HOURS = 282,            /* LAST_N_HOURS  */
    LAST_N_DAYS = 283,             /* LAST_N_DAYS  */
    EQ = 284,                      /* EQ  */
    NE = 285,                      /* NE  */
    LT = 286,                      /* LT  */
    LE = 287,                      /* LE  */
    GT = 288,                      /* GT  */
    GE = 289,                      /* GE  */
    LPAREN = 290,                  /* LPAREN  */
    RPAREN = 291,                  /* RPAREN  */
    LBRACKET = 292,                /* LBRACKET  */
    RBRACKET = 293,                /* RBRACKET  */
    COMMA = 294,                   /* COMMA  */
    DOT = 295,                     /* DOT  */
    STAR = 296,                    /* STAR  */
    SEMICOLON = 297,               /* SEMICOLON  */
    ERROR_TOKEN = 298,             /* ERROR_TOKEN  */
    STRING_LITERAL = 299,          /* STRING_LITERAL  */
    IDENTIFIER = 300,              /* IDENTIFIER  */
    NUMBER = 301                   /* NUMBER  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 32 "src/query/hl7sql.y"

    char *string;
    long long number;
    ast_node_t *node;
    ast_node_t **node_list;
    int count;

#line 118 "src/query/hl7sql.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (ast_node_t **result);


#endif /* !YY_YY_SRC_QUERY_HL7SQL_TAB_H_INCLUDED  */
