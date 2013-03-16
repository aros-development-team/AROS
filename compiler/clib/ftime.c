/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2001 function ftime().
    Function is deprecated and removed from POSIX.1-2008
*/
#include <sys/time.h>

/*****************************************************************************

    NAME */
#include <sys/timeb.h>

	int ftime(

/*  SYNOPSIS */
	struct timeb *tb)

/*  FUNCTION
        Get info on current time and timezone.

    INPUTS
        tb - Structure to fill in time, it has the following fields
            * time: time in seconds since UNIX epoch
            * millitm: milliseconds since last second
            * timezone: minutes time west of Greenwich
            * dstflag: type of daylight saving time
        millitm is currently always multiple of 1000
        dstflag is the same as from timezone information from the
        gettimeofday() function.

    RESULT
        Always returns 0.

    NOTES
        This function is deprecated and not present anymore in POSIX.1-2008.
        This function should not be used in new code and old code should
        be fixed to remove usage.
        As an alternative gettimeofday() can be used.
 
    EXAMPLE

    BUGS

    SEE ALSO
        gettimeofday()

    INTERNALS
        This function is part of libarosc.a and may be removed in the future.

******************************************************************************/
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    tb->time     = tv.tv_sec;
    tb->millitm  = tv.tv_usec*1000;
    tb->timezone = tz.tz_minuteswest;
    tb->dstflag  = tz.tz_dsttime;

    return 0;
}

