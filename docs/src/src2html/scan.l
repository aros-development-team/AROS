%{
#include "gram.h"
#include "error.h"
#include "varstr.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "util.h"
#include "emit.h"
#include "scan.h"

extern char * substr(const char *str, int front, int back);
extern char * getarg(const char *str);

#define YY_DECL 	int yylex (YYSTYPE * yylval, FILE * fh)
#define YY_USER_INIT	yyin = fh;

#define MAX_INCLUDE_DEPTH 10
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int yylineno_stack[MAX_INCLUDE_DEPTH];
char * filename_stack[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;
int bracelevel = 0;

VarString * vs;
%}

%pointer
%option noyywrap yylineno
%x example incl arg code

CODE	    \|[^\|]+\|
ARG	    [ \t\n]*[{][^}]*[}]
TEXT	    ([^\\\|{]|\n)+

%%

"\\chapter"       { return CHAPTER; }
"\\section"       { return SECTION; }
"\\subsection"      { return SUBSECTION; }
"\\begin\{example\}" { vs=createvarstring (NULL); YYBEGIN(example); }
<example>{
    "\\end"[ \t\n]*"\{example\}" { yylval->string=tostring(vs); YYBEGIN(INITIAL); return EXAMPLE; }
    "\\end"[ \t\n]*"\{"   { yyerror ("Unexpected \\end{%s}. Expected \\end{example}", getarg(yytext)); }
    "\\end"             { yyerror ("\\end without argument. Expected \\end{example}"); }
    ([^\\]*|\n)*        { appendtovarstring (vs, yytext); }
    \\			{ appendtovarstring (vs, yytext); }
}
"\\begin\{code\}" { vs=createvarstring (NULL); YYBEGIN(code); }
<code>{
    "\\end"[ \t\n]*"\{code\}" { yylval->string=tostring(vs); YYBEGIN(INITIAL); return BIGCODE; }
    "\\end"[ \t\n]*"\{"   { yyerror ("Unexpected \\end{%s}. Expected \\end{code}", getarg(yytext)); }
    "\\end"             { yyerror ("\\end without argument. Expected \\end{code}"); }
    ([^\\]*|\n)*        { appendtovarstring (vs, yytext); }
    \\			{ appendtovarstring (vs, yytext); }
}
"\\begin"[ \t\n]*"\{new\}"  { return BEGIN_NEW; }
"\\begin"         { return BEGIN; }
"\\end"           { return END; }
"\\item"          { return ITEM; }
"\\link"          { return LINK; }
"\\filename"      { return FILENAME; }
"\\fileinfo"      { return FILEINFO; }
"\\lref"          { return LREF; }
"\\label"         { return LABEL; }
"\\shell"         { return SHELL; }
"\\bold"          { return BOLD; }
"\\email"         { return EMAIL; }
"\\italics"       { return ITALICS; }
"\\smallpic"      { return SMALLPIC; }
"\\largepic"      { return LARGEPIC; }
"\\toc"           { return TOC; }
"\\theindex"      { return THEINDEX; }
"\\appendix"      { return APPENDIX; }
"\\if"            { return IF; }
"\\debug"         { return DEBUG; }
"\\par"           { return PAR; }
"\\nl"            { return NL; }
"\\ "             { return SPACE; }
"\\index"         { return INDEX; }
"\\today"         { return TODAY; }

"\\echo"{ARG}     { char * str = getarg(yytext); fputs (str, stdout); putchar ('\n'); free (str); }

"\\incverb"{ARG}  {
		    char * name = getarg(yytext);
		    FILE * fh;
		    int c;

		    fh = fopen (name, "r");

		    if (!fh)
			yyerror("Can't open include file %s: %s",
			    name, strerror (errno));

		    printf ("Including %s...\n", name);

		    while ((c = fgetc (fh)) != EOF)
		    {
			emit_char_always (c);
		    }

		    fclose (fh);
		  }

"\\include"{ARG}  {
		    char * name = getarg(yytext);

		    scan_push ();
		    yyin = fopen (name, "r");

		    if (!yyin )
			yyerror("Can't open include file %s: %s",
			    name, strerror (errno));

		    printf ("Including %s...\n", name);

		    yy_switch_to_buffer (yy_create_buffer (yyin, YY_BUF_SIZE));
		    filename = name;
		    yylineno = 1;

		    YYBEGIN(INITIAL);
		}

<INITIAL><<EOF>> {
		    if (--include_stack_ptr < 0)
		    {
			yyterminate();
		    }
		    else
		    {
			scan_pop ();
		    }
		}

\\[a-z]+	{ yyerror ("Unknown command %s", yytext); }
\\[\\&$@#{}]	{ yylval->string = substr(yytext,1,0); return TEXT; }
{CODE}		{ yylval->string = substr(yytext,1,1); return SMALLCODE; }

[ \t\n]*\{	{ vs=createvarstring (NULL); YYBEGIN(arg); bracelevel=0; }
<arg>{
    "\}"            {
			if (--bracelevel < 0)
			{
			    yylval->string=tostring(vs);
			    YYBEGIN(INITIAL);
			    return ARG;
			}
			else
			    appendtovarstring (vs, yytext);
		    }
    "\{"            {
			bracelevel ++;
			appendtovarstring (vs, yytext);
		    }
    <<EOF>>	    { yyerror ("Missing %d }", bracelevel); }
    ([^{}]|\n)*     { appendtovarstring (vs, yytext); }
}
{TEXT}		{ yylval->string = xstrdup(yytext); return TEXT; }
%%
char * getarg (const char * str)
{
    int    front, len;
    char * res;

    for (front=0; str[front]!='{' && str[front]; front++);

    if (!str[front])
	return NULL;

    front ++;

    for (len=front; str[len]!='}' && str[len]; len++);

    len -= front;

    res = xmalloc (len + 1);

    memmove (res, str+front, len);
    res[len] = 0;

    return res;
}

char * substr (const char * str, int front, int back)
{
    int    len = strlen (str);
    char * res;

    len -= front + back;

    res = xmalloc (len + 1);

    memmove (res, str+front, len);
    res[len] = 0;

    return res;
}

void yyerror (const char * fmt, ...)
{
    va_list args;

    va_start (args, fmt);

    fprintf (stderr, "Error in %s.%d:\n", filename ? filename : "*unknown*", yylineno);
    vfprintf (stderr, fmt, args);
    fputc ('\n', stderr);

    va_end (args);

    exit (10);
}

void yywarn (const char * fmt, ...)
{
    va_list args;

    va_start (args, fmt);

    fprintf (stderr, "Warning in %s.%d:\n", filename ? filename : "*unknown*", yylineno);
    vfprintf (stderr, fmt, args);
    fputc ('\n', stderr);

    va_end (args);
}

void scan_push (void)
{
    if (include_stack_ptr >= MAX_INCLUDE_DEPTH)
	yyerror ("Includes nested too deeply");

    yylineno_stack[include_stack_ptr] = yylineno;
    filename_stack[include_stack_ptr] = filename;
    include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;
}

void scan_pop (void)
{
    yy_delete_buffer (YY_CURRENT_BUFFER);
    yy_switch_to_buffer (include_stack[include_stack_ptr]);
    xfree (filename);
    filename = filename_stack[include_stack_ptr];
    yylineno = yylineno_stack[include_stack_ptr];
}
