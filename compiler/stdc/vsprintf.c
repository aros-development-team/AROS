/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C function vsprintf().
*/
/* Original source from libnix */


static int _vsprintf_uc (int c, char ** str)
{
    *(*str)++ = c;
    return 1;
}

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

	int vsprintf (

/*  SYNOPSIS */
	char	   * str,
	const char * format,
	va_list      args)

/*  FUNCTION
	Format a list of arguments and put them into the string str.

    INPUTS
	str - The formatted result is stored here
	format - A printf() format string.
	args - A list of arguments for the format string.

    RESULT
	The number of characters written.

    NOTES
	No check is made that str is large enough to contain the result.

    EXAMPLE

    BUGS

    SEE ALSO
	printf(), sprintf(), fprintf(), vprintf(), vfprintf(), snprintf(),
	vsnprintf()

    INTERNALS

******************************************************************************/
{
    int rc;

    rc = __vcformat (&str, (void *)_vsprintf_uc, format, args);

    *str = 0;

    return rc;
} /* vsprintf */
