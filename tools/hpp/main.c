#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <toollib/stdiocb.h>
#include <toollib/error.h>
#include "html.h"
#include "parse.h"
#include "var.h"
#include "db.h"
#include "func.h"
#include "main.h"

static const char version[] = "$VER: hpp 0.2 (19.11.1997)\r\n";

static int
MyStdioPutCB (StdioStream * ss, int c, CBD data)
{
    static int lastc   = '\n';
    static int size    = 0;
    static int linelen = 0;
    static int intag   = 0;
    int allowbreak;

    if (ss->out)
    {
	allowbreak = 0;

	if (c == '\n')
	{
	    linelen = 0;
	    if (c == lastc)
		return 1;
	}
	else if (!intag && isspace (c))
	{
	    allowbreak = 1;
	}

	lastc = c;

	if (linelen > 70 && allowbreak)
	{
	    linelen = 0;

	    if (allowbreak > 0)
	    {
		putc ('\n', ss->out);

		if (isspace (c))
		    return c;
	    }
	    else
	    {
		putc (c, ss->out);
		c = '\n';
	    }
	}

	size ++;
	linelen ++;

	if (size == 1024)
	{
	    int oc = atoi (Var_Get ("outputCount"));
	    char buffer[32];
	    sprintf (buffer, "%d", oc+1);
	    Var_Set ("outputCount", buffer);
	    size = 0;
	}

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

static StdioStream * output = NULL;

void WriteTo (const char * filename)
{
    if (output)
    {
	if (output->closeout)
	{
	    fclose (output->out);
	    output->closeout = 0;
	}

	if (strcmp (filename, "-"))
	{
	    output->out = fopen (filename, "w");

	    if (!output->out)
	    {
		PushStdError ("Can't open \"%s\" for writing\n", filename);
		ErrorExit (10);
	    }

	    output->closeout = 1;
	}
	else
	{
	    output->out = stdout;
	    output->closeout = 0;
	}
    }
    else
    {
	output = StdStr_New (filename, "w");

	if (!output)
	{
	    PushStdError ("Can't open \"%s\" for writing\n", filename);
	    ErrorExit (10);
	}
    }

    Var_Set ("outputName", filename);

    output->stream.put	= (CB) MyStdioPutCB;
    output->stream.puts = (CB) MyStdioPutsCB;
}

void main (int argc, char ** argv)
{
    StdioStream * ss;
    time_t tt;
    struct tm tm;
    char today[32];
    char * infiles[64];
    int    ninfiles = 0;
    char * outfile;
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
    Var_Set ("outputCount", "0");

    outfile = "-";

    for (t=1; t<argc; t++)
    {
	if (!strcmp (argv[t], "-set"))
	{
	    char * name, * value;

	    name = argv[++t];
	    value = name;

	    while (*value && *value != '=') value++;

	    if (*value)
		*value++ = 0;

	    Var_Set (name, value);
	}
	else if (!strcmp (argv[t], "-o") || !strcmp (argv[t], "-output"))
	{
	    outfile = argv[++t];
	}
	else
	{
	    infiles[ninfiles++] = argv[t];
	}
    } /* for all args */

    WriteTo (outfile);

    for (t=0; t<ninfiles; t++)
    {
	ss = StdStr_New (infiles[t], "r");

	if (!ss)
	    PrintErrorStack ();
	else
	{
	    Var_Set ("filename", infiles[t]);

	    rc = HTML_Parse ((MyStream *) ss, (MyStream *) output, NULL);

	    if (rc == T_ERROR)
	    {
		PushError ("%s:%d:", Str_GetName (ss), Str_GetLine (ss));
		PrintErrorStack ();
	    }

	    StdStr_Delete (ss);
	} /* if (ss) */
    } /* for all input files */

    Str_Put (output, '\n', NULL);
    StdStr_Delete (output);

    HTML_Exit ();
    DB_Exit ();
    Func_Exit ();
    Var_Exit ();

    ErrorExit (0);
}

