/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Return the current time in seconds.
*/

#include <dos/dos.h>
#include <proto/dos.h>

long __gmtoffset;

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
    struct DateStamp t;
    time_t	     tt;

    DateStamp (&t); /* Get timestamp */

    /*
	2922 is the number of days between 1.1.1970 and 1.1.1978 (2 leap
		years and 6 normal). The former number is the start value
		for time(), the latter the start time for the AmigaOS
		time functions.
	1440 is the number of minutes per day
	60 is the number of seconds per minute
    */
    tt = ((t.ds_Days + 2922) * 1440 + t.ds_Minute + __gmtoffset) * 60
	+ t.ds_Tick / TICKS_PER_SECOND;

    if (tloc != NULL)
	*tloc = tt;

    return tt;
} /* time */

