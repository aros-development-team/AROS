%pure_parser

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "emit.h"
#include "util.h"

#define YYLEX_PARAM	    (FILE *)fh
#define YYPARSE_PARAM	    fh
#define YYERROR_VERBOSE     1
%}

%union {
    int    number;
    char * string;
}

%left <string> TEXT SMALLCODE BIGCODE EXAMPLE
%left CHAPTER SECTION SUBSECTION LINK
%left BEGIN ITEM END FILENAME FILEINFO
%left LREF SHELL LABEL BOLD EMAIL ITALICS
%left SMALLPIC LARGEPIC TOC APPENDIX
%left DEBUG PAR NL
%left <string> ARG
%left IF
%type <string> optarg
%%
file : /* eps */ | fileparts ;

fileparts : filepart
	| fileparts filepart
	;

filepart : command
	| SMALLCODE
	{ emit (SMALLCODE, $1); xfree ($1); }
	| BIGCODE
	{ emit (BIGCODE, $1); xfree ($1); }
	| TEXT
	{ emit (TEXT, $1); xfree ($1); }
	| ARG
	{ yyerror ("Unexpected argument ARG(%s)", $1); }
	;

command : CHAPTER ARG
	{ emit (CHAPTER, $2); xfree ($2); }
	| SECTION ARG
	{ emit (SECTION, $2); xfree ($2); }
	| SUBSECTION ARG
	{ emit (SUBSECTION, $2); xfree ($2); }
	| LINK ARG ARG
	{ emit (LINK, $2, $3); xfree ($2); xfree ($3); }
	| BEGIN ARG
	{ emit (BEGIN, $2); xfree ($2); }
	| ITEM optarg
	{ emit (ITEM, $2); if ($2) xfree ($2); }
	| END ARG
	{ emit (END, $2); xfree ($2); }
	| FILENAME ARG
	{ emit (FILENAME, $2); xfree ($2); }
	| FILEINFO ARG
	{ emit (FILEINFO, $2); xfree ($2); }
	| LREF ARG ARG
	{ emit (LREF, $2, $3); xfree ($2); xfree ($3); }
	| LREF ARG TEXT { yyerror ("Missing second argument for \\lref"); }
	| LREF TEXT { yyerror ("Missing arguments for \\lref"); }
	| SHELL ARG
	{ emit (SHELL, $2); xfree ($2); }
	| LABEL ARG
	{ emit (LABEL, $2); xfree ($2); }
	| BOLD ARG
	{ emit (BOLD, $2); xfree ($2); }
	| EMAIL ARG
	{ emit (EMAIL, $2); xfree ($2); }
	| EXAMPLE
	{ emit (EXAMPLE, $1); xfree ($1); }
	| ITALICS ARG
	{ emit (ITALICS, $2); xfree ($2); }
	| SMALLPIC ARG
	{ emit (SMALLPIC, $2); xfree ($2); }
	| LARGEPIC ARG ARG
	{ emit (LARGEPIC, $2, $3); xfree ($2); xfree ($3); }
	| TOC
	{ emit (TOC); }
	| APPENDIX
	{ emit (APPENDIX); }
	| IF ARG ARG optarg
	{ emit (IF, $2, $3, $4); xfree ($2); xfree ($3); if ($4) xfree ($4); }
	| DEBUG
	{ yydebug = 1; }
	| PAR
	{ emit (PAR); }
	| NL
	{ emit (NL); }
	;

optarg	: /* eps */ { $$=NULL; }
	| ARG
	;

%%
