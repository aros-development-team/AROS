#include <ctype.h>
#include <toollib/error.h>
#include "expr.h"
#include "parse.h"
#include "var.h"

static int Expr_ParseLOr PARAMS ((const char ** str, int * result));

static int
Expr_ReadInt (const char ** str, int * result)
{
    int    value;
    char * ptr;

    value = strtol (*str, &ptr, 0);

    if (ptr == *str)
	return T_ERROR;

    *result = value;
    *str    = ptr;

    return T_OK;
}

/*
    expr ::= landexpr [ || landexpr ]
    landexpr ::= eqexpr [ && eqexpr ]
    eqexpr ::= relexpr [ ( '==' | '!=' ) relexpr ]
    relexpr ::= addexpr [ ( '<' | '<=' | '>=' | '>' ) addexpr ]
    addexpr ::= mulexpr [ ( '+' | '-' ) mulexpr ]
    mulexpr ::= unaryexpr [ ( '*' | '/' | '%' ) unaryexpr ]
    unaryexpr ::= ( '+' | '-' | '~' | '!' ) unaryExpr | primaryExpr
    primaryexpr ::= number | string | '(' expr ')'
*/

static int
Expr_ParsePrimary (const char ** str, int * result)
{
    int rc;
    const char * ptr = *str;

    while (isspace (*ptr)) ptr ++;

    *str = ptr;

    if (*ptr == '(')
    {
	*str = ptr + 1;
	rc = Expr_ParseLOr (str, result);
	ptr = *str;

	while (isspace (*ptr)) ptr ++;

	if (*ptr != ')')
	{
	    PushError ("Missing ')' in expression");
	    return T_ERROR;
	}

	*str = ptr + 1;
    }
    else
    {
	rc = Expr_ReadInt (str, result);

	if (rc != T_OK)
	{
	    PushError ("Expected integer (%s)", ptr);
	    return T_ERROR;
	}
    }

    return T_OK;
}

static int
Expr_ParseUnary (const char ** str, int * result)
{
    int val1, op, rc;
    const char * ptr = *str;

    while (isspace (*ptr)) ptr ++;

    if (*ptr == '+' || *ptr == '-' || *ptr == '~' || *ptr == '!')
    {
	op = *ptr;
	*str = ptr + 1;
    }
    else
	op = '+';

    rc = Expr_ParsePrimary (str, &val1);

    if (rc != T_OK)
	return rc;

    if (op == '-')
	*result = -val1;
    else if (op == '~')
	*result = ~val1;
    else if (op == '!')
	*result = !val1;
    else
	*result = val1;

    return T_OK;
}

static int
Expr_ParseMul (const char ** str, int * result)
{
    int val1, val2, rc;
    const char * ptr;

    rc = Expr_ParseUnary (str, &val1);

    if (rc != T_OK)
	return rc;

    ptr = *str;

    while (isspace (*ptr)) ptr ++;

    while (*ptr == '*' || *ptr == '/' || *ptr == '%')
    {
	*str = ptr + 1;

	rc = Expr_ParseUnary (str, &val2);

	if (rc != T_OK)
	    return rc;

	if (*ptr == '*')
	    val1 = (val1 * val2);
	else if (*ptr == '/')
	{
	    if (!val2)
	    {
		PushError ("Division by 0");
		return T_ERROR;
	    }

	    val1 = (val1 / val2);
	}
	else
	    val1 = (val1 % val2);

	ptr = *str;
	while (isspace (*ptr)) ptr ++;
    }

    *result = val1;

    return T_OK;
}

static int
Expr_ParseAdd (const char ** str, int * result)
{
    int val1, val2, rc;
    const char * ptr;

    rc = Expr_ParseMul (str, &val1);

    if (rc != T_OK)
	return rc;

    ptr = *str;

    while (isspace (*ptr)) ptr ++;

    while (*ptr == '+' || *ptr == '-')
    {
	*str = ptr + 1;

	rc = Expr_ParseMul (str, &val2);

	if (rc != T_OK)
	    return rc;

	if (*ptr == '+')
	    val1 = (val1 + val2);
	else
	    val1 = (val1 - val2);

	ptr = *str;
	while (isspace (*ptr)) ptr ++;
    }

    *result = val1;

    return T_OK;
}

static int
Expr_ParseRel (const char ** str, int * result)
{
    int val1, val2, rc;
    const char * ptr;

    rc = Expr_ParseAdd (str, &val1);

    if (rc != T_OK)
	return rc;

    ptr = *str;

    while (isspace (*ptr)) ptr ++;

    while (*ptr == '<' || *ptr == '>')
    {
	*str = ptr + ((ptr[1] == '=') ? 2 : 1);

	rc = Expr_ParseRel (str, &val2);

	if (rc != T_OK)
	    return rc;

	if (ptr[1] == '=')
	{
	    if (*ptr == '<')
		val1 = (val1 <= val2);
	    else
		val1 = (val1 >= val2);
	}
	else
	{
	    if (*ptr == '<')
		val1 = (val1 < val2);
	    else
		val1 = (val1 > val2);
	}

	ptr = *str;
	while (isspace (*ptr)) ptr ++;
    }

    *result = val1;

    return T_OK;
}

static int
Expr_ParseEq (const char ** str, int * result)
{
    int val1, val2, rc;
    const char * ptr;

    rc = Expr_ParseRel (str, &val1);

    if (rc != T_OK)
	return rc;

    ptr = *str;

    while (isspace (*ptr)) ptr ++;

    while ((*ptr == '=' || *ptr == '!') && ptr[1] == '=')
    {
	*str = ptr + 2;

	rc = Expr_ParseRel (str, &val2);

	if (rc != T_OK)
	    return rc;

	if (*ptr == '=')
	    val1 = (val1 == val2);
	else
	    val1 = (val1 != val2);

	ptr = *str;
	while (isspace (*ptr)) ptr ++;
    }

    *result = val1;

    return T_OK;
}

static int
Expr_ParseLAnd (const char ** str, int * result)
{
    int val1, val2, rc;
    const char * ptr;

    rc = Expr_ParseEq (str, &val1);

    if (rc != T_OK)
	return rc;

    ptr = *str;

    while (isspace (*ptr)) ptr ++;

    while (*ptr == '&' && ptr[1] == '&')
    {
	*str = ptr + 2;

	rc = Expr_ParseEq (str, &val2);

	if (rc != T_OK)
	    return rc;

	val1 = val1 && val2;

	ptr = *str;
	while (isspace (*ptr)) ptr ++;
    }

    *result = val1;

    return T_OK;
}

static int
Expr_ParseLOr (const char ** str, int * result)
{
    int val1, val2, rc;
    const char * ptr;

    rc = Expr_ParseLAnd (str, &val1);

    if (rc != T_OK)
	return rc;

    ptr = *str;

    while (isspace (*ptr)) ptr ++;

    while (*ptr == '|' && ptr[1] == '|')
    {
	*str = ptr + 2;

	rc = Expr_ParseLAnd (str, &val2);

	if (rc != T_OK)
	    return rc;

	val1 = val1 || val2;

	ptr = *str;
	while (isspace (*ptr)) ptr ++;
    }

    *result = val1;

    return T_OK;
}

#ifndef TEST
int
Expr_Parse (const char * str, int * result)
{
    String expr = Var_Subst (str);
    int    rc;
    const char * ptr;

    if (!expr)
	return T_ERROR;

    ptr = expr->buffer;

    rc = Expr_ParseLOr (&ptr, result);

    if (rc == T_OK && *ptr)
    {
	PushError ("Dangling text after expession: %s", ptr);
	rc = T_ERROR;
    }

    VS_Delete (expr);

    return rc;
}
#endif /* !TEST */

#ifdef TEST
#include <stdio.h>

int
main (int argc, char ** argv)
{
    int val, rc, t;
    const char * ptr;

    for (t=1; t<argc; t++)
    {
	ptr = argv[t];
	rc = Expr_ParseLOr (&ptr, &val);

	if (rc != T_OK)
	{
	    PushError ("Error parsing %s", argv[t]);
	    PrintErrorStack ();
	}
	else if (*ptr)
	{
	    fprintf (stderr, "Dangling text after expr: %s", ptr);
	}
	else
	{
	    printf ("Result of %s: %d\n", argv[t], val);
	}
    }

    ErrorExit (0);
}

#endif /* TEST */
