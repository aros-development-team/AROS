/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Return the current time in seconds.
*/

/*****************************************************************************

    NAME */
#include <time.h>

	char * ctime (

/*  SYNOPSIS */
	const time_t * tt)

/*  FUNCTION
	The ctime() function converts the broken-down time value tt
	into a string with this format:

	    "Wed Jun 30 21:49:08 1993\n"

	The return value points to a statically allocated string which
	might be overwritten by subsequent calls to any of the date and
	time functions.

    INPUTS
	tt - Convert this time.

    RESULT
	A statically allocated buffer with the converted time. Note that
	there is a newline at the end of the buffer and that the contents
	of the buffer might get lost with the call of any of the date
	and time functions.

    NOTES

    EXAMPLE
	time_t tt;
	char * str;

	// Get time
	time (&tt);

	// Convert to string
	str = ctime (&tt);

    BUGS

    SEE ALSO
	time(), asctime(), localtime()

    INTERNALS

******************************************************************************/
{
    return asctime (localtime (tt));
} /* ctime */
