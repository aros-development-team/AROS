/***************************************
  $Header$

  C Cross Referencing & Documentation tool. Version 1.5e.

  The function protypes that are missing from the lex and yacc output.
  ******************/ /******************
  Written by Andrew M. Bishop

  This file Copyright 1995,96,99,2002,03 Andrew M. Bishop
  It may be distributed under the GNU Public License, version 2, or
  any higher version.  See section COPYING of the GNU Public license
  for conditions under which this file may be redistributed.
  ***************************************/

#ifndef PARSE_YY_H
#define PARSE_YY_H   /*+ To stop multiple inclusions. +*/

#include <stdio.h>
#include <stdlib.h>

/*+ Debugging output or not?

  YYDEBUG=0 : No debugging at all, smaller, faster code.

  YYDEBUG=1 : Debugging output only when parsing fails.

  YYDEBUG=2 : Debugging output for each symbol from lex().

  YYDEBUG=3 : Full yacc debugging output, state transitions etc.
+*/
#define YYDEBUG 1

/* Only the #define for YYDEBUG above is user editable,
   those below are for fixing up yacc/lex only. */

/*+ Some versions of yacc (Sun) don't allow YYSTYPE to be #defined as char* +*/
typedef char* yystype;

/*+ Semantic type is a char* +*/
#define YYSTYPE yystype

/*+ use prototypes in function declarations +*/
#define YY_USE_PROTOS

/*+ the "const" storage-class-modifier is valid +*/
#define YY_USE_CONST

#if YYDEBUG

/*+ Turn the debugging output on and off. +*/
extern int yydebug;

/*+ Define verbose errors in Bison generated file +*/
#define YYERROR_VERBOSE 1

/*+ This is needed in the Bison generated code. +*/
#define xmalloc malloc

/*+ Print the semantic value +*/
#define YYPRINT(file,type,value)  printf(" '%s'",value)

#endif

/* Extern variables */

/*+ The name of the current file. +*/
extern char* parse_file;

/*+ The current line number in the file. +*/
extern int parse_line;

/*+ For communication between the lex and yacc code. +*/
extern YYSTYPE yylval;

/*+ For communication between the lex and yacc code. +*/
extern FILE *yyin;

/* Global functions */

int yylex(void);

int yyparse(void);
void yyrestart (FILE *input_file);

/* yywrap() function */

#define YY_SKIP_YYWRAP 1 /* Don't use yywrap prototype */

#ifndef yywrap

/*+ Needed in lex but does nothing. +*/
#define yywrap() 1

#endif

#endif
