/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Query the current time and/or timezone.
*/

#include <proto/exec.h>
#include <proto/timer.h>
#include <exec/types.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>

#include <time.h>
#include <errno.h>

#include "__posixc_time.h"

/*****************************************************************************

    NAME */
#include <sys/time.h>

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
	stdc.library/ctime(), stdc.library/asctime(), stdc.library/localtime(),
        stdc.library/time()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase = (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (!TimerBase)
        __init_timerbase(PosixCBase);

    if (tv)
    {
        if (TimerBase)
        {
            GetSysTime(tv);

            /* Adjust with the current timezone, stored in minutes west of GMT */
            tv->tv_sec += (2922 * 1440 + __stdc_gmtoffset()) * 60;
        }
        else
        {
            errno = EACCES;
            return -1;
        }
    }

    if (tz)
    {
	tz->tz_minuteswest = __stdc_gmtoffset();
	/* FIXME: set tz->tz_dsttime */
	tz->tz_dsttime	   = DST_NONE;
    }

    return 0;
} /* gettimeofday */
