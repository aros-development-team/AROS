#include <stdio.h>
#include <toollib/stdiocb.h>
#include <toollib/error.h>
#include "html.h"
#include "parse.h"
#include "var.h"
#include "db.h"

void main (int argc, char ** argv)
{
    StdioStream * ss;
    StdioStream * out;

    int t, rc;

    Var_Init ();
    DB_Init ();
    HTML_Init ();

    Var_Set ("outputFormat", "html");

    out = StdStr_New ("-", "w");

    for (t=1; t<argc; t++)
    {
	ss = StdStr_New (argv[t], "r");

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
    Var_Exit ();

    ErrorExit (0);
}

