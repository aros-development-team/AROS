/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <errno.h>
#include <toollib/error.h>
#include <toollib/stdiocb.h>

static int
StdioGetCB (StdioStream * ss, int dummy, CBD data)
{
    if (ss->in)
    {
	int c = getc (ss->in);

	if (c == '\n')
	    Str_NextLine (ss);

	return c;
    }

    errno = EINVAL;
    return -1;
}

static int
StdioUngetCB (StdioStream * ss, int c, CBD data)
{
    if (ss->in)
	return ungetc (c, ss->in);

    errno = EINVAL;
    return -1;
}

static int
StdioPutCB (StdioStream * ss, int c, CBD data)
{
    if (ss->out)
	return putc (c, ss->out);

    errno = EINVAL;
    return -1;
}

static int
StdioPutsCB (StdioStream * ss, const char * str, CBD data)
{
    if (ss->out)
	return fputs (str, ss->out);

    errno = EINVAL;
    return -1;
}

StdioStream *
StdStr_New (const char * path, const char * mode)
{
    StdioStream * ss = new (StdioStream);
    FILE * fh = NULL;

    if (strcmp (path, "-"))
    {
	fh = fopen (path, mode);

	if (!fh)
	{
	    PushStdError ("Can't open \"%s\" with mode \"%s\"\n", path, mode);
	    return NULL;
	}
    }

    Str_Init (&ss->stream, path);

    ss->stream.get   = (CB) StdioGetCB;
    ss->stream.unget = (CB) StdioUngetCB;
    ss->stream.put   = (CB) StdioPutCB;
    ss->stream.puts  = (CB) StdioPutsCB;

    if (strchr (mode, 'r'))
    {
	if (strcmp (path, "-"))
	{
	    ss->in = fh;
	    ss->closein = 1;
	}
	else
	{
	    ss->in = stdin;
	    ss->closein = 0;
	}
    }
    else
    {
	ss->in = NULL;
	ss->closein = 0;
    }

    if (strchr (mode, 'w') || strchr (mode, 'a'))
    {
	if (strcmp (path, "-"))
	{
	    ss->out = fh;
	    ss->closeout = 1;
	}
	else
	{
	    ss->out = stdout;
	    ss->closeout = 0;
	}
    }
    else
    {
	ss->out = NULL;
	ss->closeout = 0;
    }

    if (ss->in && ss->out)
    {
	ss->closeout = 0;
    }

    return ss;
}

void
StdStr_Delete (StdioStream * ss)
{
    if (ss->closein)
	fclose (ss->in);

    if (ss->closeout)
	fclose (ss->out);

    Str_Delete (&ss->stream);

    xfree (ss);
}

