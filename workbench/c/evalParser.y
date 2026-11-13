/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bison file for the Eval grammar.
    Lang: English
*/

%{
#define YYSTYPE  int

#include  <ctype.h>
#include  <math.h>
#include  <string.h>
#include  <stdio.h>

char *text;

extern int yylval;
static void yyerror(char *s);
static int intPow(int x, int y);
static int yylex();

int g_result;

%}

%token NUM
%left 'l' 'r'
%left 'e'
%left '|'
%left 'x'
%left '&'
%left '-' '+'
%left '*' '/' '%'
%left NEG '~'
%right '^'


%%

val: expr
{
    g_result = $1;
}
;

expr: NUM                  { $$ = $1;         }
    | expr 'l' expr        { $$ = $1 << $3;   }
    | expr 'r' expr        { $$ = $1 >> $3;   }
    | expr 'e' expr        { $$ = ($1 & $3) | (~$1 & ~$3); }
    | expr '|' expr        { $$ = $1 | $3;    }
    | expr 'x' expr        { $$ = $1 ^ $3;    }
    | expr '&' expr        { $$ = $1 & $3;    }
    | expr '+' expr        { $$ = $1 + $3;    }
    | expr '-' expr        { $$ = $1 - $3;    }
    | expr '*' expr        { $$ = $1 * $3;    }
    | expr '/' expr        { $$ = $1 / $3;    }
    | expr '%' expr        { $$ = $1 % $3;    }
    | '-' expr  %prec NEG  { $$ = -$2;        }
    | '~' expr             { $$ = ~$2;        }
    | expr '^' expr        { $$ = intPow($1, $3); }
    | '(' expr ')'         { $$ = $2;         }
;

%%

static int yylex()
{
    int c;
    
    do
    {
	c = *text++;
    } while (c == ' ' || c == '\t');

    if (c == '0')
    {
	switch (*text)
	{
	case 'x':
	    sscanf(text + 1, "%x", &yylval);

	    text++;

	    while ((*text >= '0' && *text <= '9') ||
		   (*text >= 'a' && *text <= 'f') ||
		   (*text >= 'A' && *text <= 'F'))
	    {
		text++;
	    }

	    return NUM;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	    sscanf(text - 1, "%o", &yylval);

	    while ((*text >= '0' && *text <= '7'))
	    {
		text++;
	    }

	    return NUM;

	case '8':
	case '9':
	    /* Skip 08... and 09... constructions and let them be ordinary
	       decimal stuff */
	    break;

	default:
	    /* It was just the constant 0 */
	    yylval = 0;
	    
	    return NUM;
	}

    }

    if (c == '#')
    {
	int d = *text++;

	if (d == 'x')
	{
	    sscanf(text, "%x", &yylval);
	    
	    while ((*text >= '0' && *text <= '9') ||
		   (*text >= 'a' && *text <= 'f') ||
		   (*text >= 'A' && *text <= 'F'))
	    {
		text++;
	    }
	    
	    return NUM;
	}
	else if (isdigit(d))
	{
	    sscanf(text - 1, "%o", &yylval);

	    while ((*text >= '0' && *text <= '7'))
	    {
		text++;
	    }

	    return NUM;	    
	}
	else
	{
	    yyerror("Lexer error");
	}
    }


    if (c == '\'')
    {
	yylval = *text++;

	return NUM;
    }

    if (isdigit(c))
    {
	sscanf(text - 1, "%i", &yylval);

	while (isdigit(*text))
	{
	    text++;
	}

	return NUM;
    }	

    if (isalpha(c))
    {
	int   i = 1;
	char *textCopy = text - 1;

	while (isalpha(text[i]))
	{
	    i++;
	}

	text += i;

	if (strncasecmp(textCopy, "m", i) == 0 ||
	    strncmp(textCopy, "mod", i) == 0)
	{
	    return '%';
	}
	
	if (strncmp(textCopy, "xor", i) == 0 ||
	    strncasecmp(textCopy, "x", i) == 0)
	{
	    return 'x';
	}

	if (strncmp(textCopy, "eqv", i) == 0 ||
	    strncasecmp(textCopy, "e", i) == 0)
	{
	    return 'e';
	}

	if (strncmp(textCopy, "lsh", i) == 0 ||
	    strncasecmp(textCopy, "l", i) == 0)
	{
	    return 'l';
	}

	if (strncmp(textCopy, "rsh", i) == 0 ||
	    strncasecmp(textCopy, "r", i) == 0)
	{
	    return 'r';
	}

	yyerror("Lexing error");
    }

    return c;
}
 
 
static int intPow(int x, int y)
{
    int result = 1;

    while (y > 0)
    {
	result *= x;
	y--;
    }

    return result;
}


static void yyerror(char *s)
{
    printf("%s\n", s);
}


#if 0

int main()
{
    
    text = "(1 lsh 4) mod 2";

    if (yyparse() == 1)
    {
	printf("Parse error\n");
    }
    else
    {
	printf("The answer is %i\n", g_result);
    }

    return 0;
}

#endif
