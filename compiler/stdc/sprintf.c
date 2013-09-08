/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function sprintf().
*/

#define _LIBC_KERNEL_

/*****************************************************************************

    NAME */
#include <stdio.h>

	int sprintf (

/*  SYNOPSIS */
	char	   * str,
	const char * format,
	...)

/*  FUNCTION
	Formats a list of arguments and writes them into the string str.

    INPUTS
	str - The formatted string is written into this variable. You
		must make sure that it is large enough to contain the
		result.
	format - Format string as described above
	... - Arguments for the format string

    RESULT
	The number of characters written into the string.

    NOTES
	No checks are made that str is large enough for the result.

    EXAMPLE

    BUGS

    SEE ALSO
	fprintf(), vprintf(), vfprintf(), snprintf(), vsprintf(),
	vsnprintf()

    INTERNALS

*****************************************************************************/
{
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vsprintf (str, format, args);

    va_end (args);

    return retval;
} /* sprintf */

