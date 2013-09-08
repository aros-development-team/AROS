/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Return the current time in seconds, reentrant.
*/

/*****************************************************************************

    NAME */
#include <time.h>

	char * ctime_r (

/*  SYNOPSIS */
	const time_t * tt,
        char * buf)

/*  FUNCTION
	The ctime_r() function converts the time value tt into a string with
        this format:

	    "Wed Jun 30 21:49:08 1993\n"

    INPUTS
	tt - Convert this time.
        buf - Buffer of at least 26 characters to store the string in

    RESULT
        The pointer passed in buf, containing the converted time. Note that
        there is a newline at the end of the buffer.

    NOTES

    EXAMPLE
	time_t tt;
	char str[26];

	// Get time
	time (&tt);

	// Convert to string
	ctime (&tt, str);

    BUGS

    SEE ALSO
	time(), asctime_r(), gmtime_r(), localtime_r()

    INTERNALS

******************************************************************************/
{
    struct tm tm;
    return asctime_r (localtime_r (tt, &tm), buf);
} /* ctime */
