/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
	into a string.

        See ctime_r() for details.

    INPUTS
	tt - Convert this time.

    RESULT
	A statically allocated buffer with the converted time. Note that
        the contents of the buffer might get lost with the call of any of the
        date and time functions.

    NOTES
        This function must not be used in a shared library or
        in a threaded application. Use ctime_r() instead.

    EXAMPLE
	time_t tt;
	char * str;

	// Get time
	time (&tt);

	// Convert to string
	str = ctime (&tt);

    BUGS

    SEE ALSO
	time(), asctime(), gmtime(), localtime()

    INTERNALS

******************************************************************************/
{
    return asctime (localtime (tt));
} /* ctime */
