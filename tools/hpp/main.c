#include <stdio.h>
#include <time.h>
#include <toollib/stdiocb.h>
#include <toollib/error.h>
#include "html.h"
#include "parse.h"
#include "var.h"
#include "db.h"
#include "func.h"

static const char version[] = "$VER: hpp 0.2 (19.11.1997)\r\n";

static int
MyStdioPutCB (StdioStream * ss, int c, CBD data)
{
    static int lastc = '\n';

    if (ss->out)
    {
	if (c == '\n' && c == lastc)
	    return 1;

	lastc = c;

	return putc (c, ss->out);
    }

    errno = EINVAL;
    return -1;
}

static int
MyStdioPutsCB (StdioStream * ss, const char * str, CBD data)
{
    if (ss->out)
    {
	int rc = 0;

	while (*str && (rc = CallCB (ss->stream.put, ss, *str, data)) != -1)
	    str ++;

	return rc;
    }

    errno = EINVAL;
    return -1;
}

void main (int argc, char ** argv)
{
    StdioStream * ss;
    StdioStream * out;
    time_t tt;
    struct tm tm;
    char today[32];
    int t, rc;

    time (&tt);
    tm = *localtime (&tt);
    strftime (today, sizeof (today), "%d.%m.%Y", &tm);

    Var_Init ();
    Func_Init ();
    DB_Init ();
    HTML_Init ();

    DB_Add ("filenames", "filenames.db");

    Var_Set ("outputFormat", "html");
    Var_Set ("today", today);

    out = StdStr_New ("-", "w");
    out->stream.put  = (CB) MyStdioPutCB;
    out->stream.puts = (CB) MyStdioPutsCB;

    for (t=1; t<argc; t++)
    {
	ss = StdStr_New (argv[t], "r");

	Var_Set ("filename", argv[t]);

	if (!ss)
	    PrintErrorStack ();
	else
	{
	    rc = HTML_Parse ((MyStream *) ss, (MyStream *) out, NULL);

	    if (rc == T_ERROR)
	    {
		PushError ("%s:%d:", Str_GetName (ss), Str_GetLine (ss));
		PrintErrorStack ();
	    }

	    StdStr_Delete (ss);
	} /* if (ss) */
    } /* for all args */

    HTML_Exit ();
    DB_Exit ();
    Func_Exit ();
    Var_Exit ();

    ErrorExit (0);
}

