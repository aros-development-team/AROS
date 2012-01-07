/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into a string.
*/

/*****************************************************************************

    NAME */
#include <time.h>

	struct tm * localtime (

/*  SYNOPSIS */
	const time_t * tt)

/*  FUNCTION
	Splits the system time in seconds into a structure.

        See localtime_r() for details.

    INPUTS
	tt - A time in seconds from the 1. Jan 1970

    RESULT
	A statically allocated buffer with the broken up time. Note that
	the contents of the buffer might get lost with the call of any of
	the date and time functions.

    NOTES
        This function must not be used in a shared library or
        in a threaded application. Use localtime_r() instead.

    EXAMPLE
	time_t	    tt;
	struct tm * tm;

	// Get time
	time (&tt);

	// Break time up
	tm = localtime (&tt);

    BUGS

    SEE ALSO
	time(), ctime(), asctime(), gmtime()

    INTERNALS

******************************************************************************/
{
    static struct tm tm;

    return localtime_r (tt, &tm);
} /* localtime */
