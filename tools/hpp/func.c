#include <stdio.h>
#include <toollib/error.h>
#include "var.h"
#include "parse.h"
#include "expr.h"

static String DiffDateCB (const char ** args, int dummy, CBD data);
static String ExprCB	(const char ** args, int dummy, CBD data);

static String
DiffDateCB (const char ** args, int dummy, CBD data)
{
    int day1, mon1, year1, day2, mon2, year2;
    int val1, val2, rc;
    char buffer[32];

    if (!args[0] || !args[1])
    {
	PushError ("$diffdate(): Expecting two args");
	return NULL;
    }

    rc = sscanf (args[0], "%d.%d.%d", &day1, &mon1, &year1);

    if (rc != 3)
    {
	PushError ("$diffdate(): First argument is no date (%s)", args[0]);
	return NULL;
    }

    rc = sscanf (args[1], "%d.%d.%d", &day2, &mon2, &year2);

    if (rc != 3)
    {
	PushError ("$diffdate(): Second argument is no date (%s)", args[1]);
	return NULL;
    }

    val1 = day1 + (mon1 + year1*12)*30;
    val2 = day2 + (mon2 + year2*12)*30;

    sprintf (buffer, "%d", val2 - val1);

    return VS_New (buffer);
}

static String
ExprCB (const char ** args, int dummy, CBD data)
{
    int rc, value;
    char buffer[32];

    if (!args[0])
    {
	PushError ("$expr(): Expecting one arg");
	return NULL;
    }

    rc = Expr_Parse (args[0], &value);

    if (rc != T_OK)
    {
	PushError ("Error parsing expression %s", args[0]);
	return NULL;
    }

#ifdef HAVE_VSNPRINT
    snprintf (buffer, sizeof (buffer), "%d", value);
#else
    sprintf (buffer, "%d", value);
#endif

    return VS_New (buffer);
}

void
Func_Init (void)
{
    Var_Init ();

    Func_Add ("diffdate", (CB) DiffDateCB, NULL);
    Func_Add ("expr", (CB) ExprCB, NULL);
}

void
Func_Exit (void)
{
    Var_Exit ();
}

