/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <errno.h>
#include <toollib/error.h>
#include <toollib/stringcb.h>

static int
StringGetCB (StringStream * ss, int dummy, CBD data)
{
    int c;

    if (ss->pos == ss->max)
    {
	errno = 0;
	c = -1;
    }
    else
	c = ss->string[ss->pos++];

    if (c == '\n')
	Str_NextLine (ss);

    return c;
}

static int
StringUngetCB (StringStream * ss, int c, CBD data)
{
    if (!ss->pos)
    {
	errno = EINVAL;
	c = -2;
    }
    else if (c != -1)
    {
	ss->pos --;

	if (c != ss->string[ss->pos])
	{
	    errno = EINVAL;
	    c = -2;
	}
    }
    else /* EOF */
    {
	if (ss->pos != ss->max)
	{
	    errno = EINVAL;
	    c = -2;
	}
    }

    return c;
}

static int
StringPutCB (StringStream * ss, int c, CBD data)
{
    if (!ss->out)
	ss->out = VS_New (NULL);

    VS_AppendChar (ss->out, c);

    return c;
}

static int
StringPutsCB (StringStream * ss, const char * str, CBD data)
{
    if (!ss->out)
	ss->out = VS_New (NULL);

    VS_AppendString (ss->out, str);

    return 1;
}

StringStream *
StrStr_New (const char * string)
{
    StringStream * ss = new (StringStream);

    Str_Init (&ss->stream, "string");

    ss->stream.get   = (CB) StringGetCB;
    ss->stream.unget = (CB) StringUngetCB;
    ss->stream.put   = (CB) StringPutCB;
    ss->stream.puts  = (CB) StringPutsCB;

    ss->string = string;
    ss->out    = NULL;
    ss->pos    = 0;
    ss->max    = strlen (string);

    return ss;
}

void
StrStr_Delete (StringStream * ss)
{
    if (ss->out)
	VS_Delete (ss->out);

    Str_Delete (&ss->stream);

    xfree (ss);
}

