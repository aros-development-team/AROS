/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Query the current time and/or timezone.
*/

#include <sys/time.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include "__time.h"

long __gmtoffset;

/*****************************************************************************

    NAME */
#include <sys/time.h>
#include <unistd.h>

	int gettimeofday (

/*  SYNOPSIS */
	struct timeval	* tv,
	struct timezone * tz)

/*  FUNCTION
	Return the current time and/or timezone.

    INPUTS
	tv - If this pointer is non-NULL, the current time will be
		stored here. The structure looks like this:

		struct timeval
		{
		    long tv_sec;	// seconds
		    long tv_usec;	// microseconds
		};

	tz - If this pointer is non-NULL, the current timezone will be
		stored here. The structure looks like this:

		struct timezone
		{
		    int  tz_minuteswest; // minutes west of Greenwich
		    int  tz_dsttime;	 // type of dst correction
		};

		With daylight savings times defined as follows :

		DST_NONE	// not on dst
		DST_USA 	// USA style dst
		DST_AUST	// Australian style dst
		DST_WET 	// Western European dst
		DST_MET 	// Middle European dst
		DST_EET 	// Eastern European dst
		DST_CAN 	// Canada
		DST_GB		// Great Britain and Eire
		DST_RUM 	// Rumania
		DST_TUR 	// Turkey
		DST_AUSTALT	// Australian style with shift in 1986

		And the following macros are defined to operate on this :

		timerisset(tv) - TRUE if tv contains a time

		timercmp(tv1, tv2, cmp) - Return the result of the
			comparison "tv1 cmp tv2"

		timerclear(tv) - Clear the timeval struct

    RESULT
	The number of seconds.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE
	struct timeval tv;

	// Get the current time and print it
	gettimeofday (&tv, NULL);

	printf ("Seconds = %ld, uSec = %ld\n", tv->tv_sec, tv->tv_usec);

    BUGS

    SEE ALSO
	ctime(), asctime(), localtime(), time()

    INTERNALS

******************************************************************************/
{
    struct DateStamp t;

    DateStamp (&t); /* Get timestamp */

    if (tv)
    {
	tv->tv_sec = (t.ds_Days * 24*60 + t.ds_Minute + __gmtoffset) * 60 +
	             t.ds_Tick / TICKS_PER_SECOND + OFFSET_FROM_1970;
	tv->tv_usec = (t.ds_Tick % TICKS_PER_SECOND) *
		      (1000000 / TICKS_PER_SECOND);
    }

    if (tz)
    {
	tz->tz_minuteswest = __gmtoffset;
#warning FIXME: set tz->tz_dsttime
	tz->tz_dsttime	   = DST_NONE;
    }

    return 0;
} /* gettimeofday */

