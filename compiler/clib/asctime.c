/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into a string.
*/

/*****************************************************************************

    NAME */
#include <time.h>

	char * asctime (

/*  SYNOPSIS */
	const struct tm * tm)

/*  FUNCTION
	The asctime() function converts the broken-down time value tm
	into a string with this format:

	    "Wed Jun 30 21:49:08 1993\n"

	The return value points to a statically allocated string which
	might be overwritten by subsequent calls to any of the date and
	time functions.

    INPUTS
	tm - The broken down time

    RESULT
	A statically allocated buffer with the converted time. Note that
	there is a newline at the end of the buffer and that the contents
	of the buffer might get lost with the call of any of the date
	and time functions.

    NOTES

    EXAMPLE
	time_t	    tt;
	struct tm * tm;
	char	  * str;

	// Get time
	time (&tt);

	// Break time up
	tm = localtime (&tt);

	// Convert to string
	str = asctime (tm);

    BUGS

    SEE ALSO
	time(), ctime(), localtime()

    INTERNALS

******************************************************************************/
{
    static char buffer[26];

    strftime (buffer, sizeof (buffer), "%C\n", tm);

    return buffer;
} /* asctime */
