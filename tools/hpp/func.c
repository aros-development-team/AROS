#include <stdio.h>
#include <string.h>
#include <toollib/error.h>
#include "var.h"
#include "parse.h"
#include "expr.h"

static String DiffDateCB (const char ** args, int dummy, CBD data);
static String ExprCB	 (const char ** args, int dummy, CBD data);
static String BasenameCB (const char ** args, int dummy, CBD data);
static String StrcmpCB	 (const char ** args, int dummy, CBD data);
static String StrcasecmpCB (const char ** args, int dummy, CBD data);

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

    if (!args[0] || args[1])
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

static String
BasenameCB (const char ** args, int dummy, CBD data)
{
    const char * ptr;
    const char * begin;

    if (!args[0] || args[1])
    {
	PushError ("$basename(): Expecting one arg");
	return NULL;
    }

    begin = strrchr (args[0], '/');

    if (!begin)
	begin = args[0];
    else
	begin ++;

    ptr = strrchr (begin, '.');

    if (ptr)
    {
	return VS_SubString (begin, 0, (long)ptr-(long)begin);
    }

    return VS_New (begin);
}

static String
StrcmpCB (const char ** args, int dummy, CBD data)
{
    int  rc;
    char buffer[32];

    if (!args[0] || !args[1] || args[2])
    {
	PushError ("$diffdate(): Expecting two args");
	return NULL;
    }

    rc = strcmp (args[0], args[1]);

    sprintf (buffer, "%d", rc);

    return VS_New (buffer);
}

static String
StrcasecmpCB (const char ** args, int dummy, CBD data)
{
    int  rc;
    char buffer[32];

    if (!args[0] || !args[1] || args[2])
    {
	PushError ("$diffdate(): Expecting two args");
	return NULL;
    }

    rc = strcasecmp (args[0], args[1]);

    sprintf (buffer, "%d", rc);

    return VS_New (buffer);
}

void
Func_Init (void)
{
    Var_Init ();

    Func_Add ("diffdate", (CB) DiffDateCB, NULL);
    Func_Add ("expr", (CB) ExprCB, NULL);
    Func_Add ("basename", (CB) BasenameCB, NULL);
    Func_Add ("strcmp", (CB) StrcmpCB, NULL);
    Func_Add ("strcasecmp", (CB) StrcasecmpCB, NULL);
}

void
Func_Exit (void)
{
    Var_Exit ();
}

