/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <toollib/error.h>

void
Error (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    fprintf (stderr, "Error: ");
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    va_end (args);
}

void
Warn (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    fprintf (stderr, "Warning: ");
    vfprintf (stderr, fmt, args);
    fprintf (stderr, "\n");
    va_end (args);
}

void
StdError (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    fprintf (stderr, "Error: ");
    vfprintf (stderr, fmt, args);
    fprintf (stderr, ": %s\n", strerror (errno));
    va_end (args);
}

void
StdWarn (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    fprintf (stderr, "Warning: ");
    vfprintf (stderr, fmt, args);
    fprintf (stderr, ": %s\n", strerror (errno));
    va_end (args);
}

static int    ErrorSP = 0;
static char * ErrorStack[32];

void
PushMsg (const char * pre, const char * fmt, va_list args, const char * post)
{
    char buffer[256];
    int len, rest;

    strcpy (buffer, pre);
    len = strlen (buffer);
    strcpy (buffer+len, ": ");
    len += 2;

    rest = sizeof (buffer) - len;

#ifdef HAVE_VSNPRINTF
    len = vsnprintf (buffer+len, rest, fmt, args);
#else
    len = vsprintf (buffer+len, fmt, args);
#endif

    rest -= len;

    if (post)
    {
	len = strlen (post);

	if (rest >= 2)
	{
	    strcat (buffer, ": ");
	    rest -= 2;
	}

	if (len < rest)
	{
	    strcpy (buffer + sizeof(buffer) - rest, post);
	}
	else
	{
	    strncpy (buffer + sizeof(buffer) - rest, post, rest-1);
	    buffer[sizeof (buffer) - 1] = 0;
	}
    }

    ErrorStack[ErrorSP++] = xstrdup (buffer);
}

void
PrintErrorStack (void)
{
    while (ErrorSP--)
	fprintf (stderr, "%s\n", ErrorStack[ErrorSP]);

    ErrorSP = 0;
}

void
ClearErrorStack (void)
{
    ErrorSP = 0;
}

void
PushError (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    PushMsg ("Error", fmt, args, NULL);
    va_end (args);
}

void
PushWarn (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    PushMsg ("Warning", fmt, args, NULL);
    va_end (args);
}

void
PushStdError (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    PushMsg ("Error", fmt, args, strerror (errno));
    va_end (args);
}

void
PushStdWarn (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    PushMsg ("Warning", fmt, args, strerror (errno));
    va_end (args);
}

void
ErrorExit (int ec)
{
    PrintErrorStack ();

    exit (ec);
}

