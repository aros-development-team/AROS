/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into a string, reentrant.
*/

/*****************************************************************************

    NAME */
#include <time.h>

	char * asctime_r (

/*  SYNOPSIS */
	const struct tm * tm,
        char * buf)

/*  FUNCTION
	The asctime_r() function converts the broken-down time value tm
	into a string with this format:

	    "Wed Jun 30 21:49:08 1993\n"

    INPUTS
	tm - The broken down time
        buf - Buffer of at least 26 characters to store the string in

    RESULT
        The pointer passed in buf, containing the converted time. Note that
        there is a newline at the end of the buffer.

    NOTES

    EXAMPLE
	time_t	  tt;
	struct tm tm;
	char	  str[26];

	// Get time
	time (&tt);

	// Break time up
	localtime (&tt, &tm);

	// Convert to string
	asctime (&tm, str);

    BUGS

    SEE ALSO
	time(), ctime_r(), gmtime_r(), localtime_r()

    INTERNALS

******************************************************************************/
{
    strftime (buf, 26, "%C\n", tm);

    return buf;
} /* asctime */
