/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Return the current time in seconds.
*/

#include <sys/time.h>

/*****************************************************************************

    NAME */
#include <time.h>

	time_t time (

/*  SYNOPSIS */
	time_t * tloc)

/*  FUNCTION
       time() returns the time since 00:00:00 GMT, January 1, 1970,
       measured in seconds.

    INPUTS
	tloc - If this pointer is non-NULL, then the time is written into
		this variable as well.

    RESULT
	The number of seconds.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE
	time_t tt1, tt2;

	// tt1 and tt2 are the same
	tt1 = time (&tt2);

	// This is valid, too
	tt1 = time (NULL);

    BUGS

    SEE ALSO
	ctime(), asctime(), localtime()

    INTERNALS

******************************************************************************/
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (tloc)
        *tloc = tv.tv_sec;
    return tv.tv_sec;
} /* time */

