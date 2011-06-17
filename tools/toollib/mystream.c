/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <toollib/mystream.h>
#include <toollib/error.h>

int
Str_Init (MyStream * ms, const char * name)
{
    memset (ms, 0, sizeof (MyStream));

    ms->line = 1;
    ms->name = xstrdup (name);

    return 1;
}

void
Str_Delete (MyStream * ms)
{
    xfree (ms->name);
}

int
Str_Puts (MyStream * ms, const char * str, CBD data)
{
    int c;

    if (ms->puts)
	c = CallCB (ms->puts, ms, str, data);
    else
    {
	c = 0;

	while (*str && (c = Str_Put(ms,NULL + *str,data)) > 0)
	    str ++;
    }

    return c;
}

void
Str_PushError (MyStream * ms, const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    PushMsg ("    ", fmt, args, NULL);
    va_end (args);
    PushError ("in %s:%d:", Str_GetName(ms), Str_GetLine(ms));
}

void
Str_PushWarn (MyStream * ms, const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    PushMsg ("    ", fmt, args, NULL);
    va_end (args);
    PushWarn ("in %s:%d:", Str_GetName(ms), Str_GetLine(ms));
}


