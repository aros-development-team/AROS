/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into a string, reentrant.
*/

extern long __gmtoffset;

/*****************************************************************************

    NAME */
#include <time.h>

	struct tm * localtime_r (

/*  SYNOPSIS */
	const time_t * tt,
        struct tm * tm)

/*  FUNCTION
	Splits the system time in seconds into a structure.
	The members of the tm structure are:

	\begin{description}
	\item {tm_sec} The number of seconds after the minute, normally in
		the range 0 to 59, but can be up to 61 to allow for leap
		seconds.

	\item{tm_min} The number of minutes after the hour, in the range 0
		to 59.

	\item{tm_hour} The number of hours past midnight, in the range 0 to
		23.

	\item{tm_mday} The day of the month, in the range 1 to 31.

	\item{tm_mon} The number of months since January, in the range 0 to
		11.

	\item{tm_year} The number of years since 1900.

	\item{tm_wday} The number of days since Sunday, in the range 0 to
		6.

	\item{tm_yday} The number of days since January 1, in the range  0
		to 365.

	\item{tm_isdst} A flag that indicates whether daylight saving time
		is in effect at the time described. The value is positive
		if daylight saving time is in effect, zero if it is not,
		and negative if the information is not available.

	\end{description}

    INPUTS
	tt - A time in seconds from the 1. Jan 1970
        tm - A struct tm to store the result in

    RESULT
        The pointer passed in tm.

    NOTES

    EXAMPLE
	time_t	  tt;
	struct tm tm;

	// Get time
	time (&tt);

	// Break time up
	localtime_r (&tt, &tm);

    BUGS

    SEE ALSO
	time(), ctime_r(), asctime_r(), gmtime_r()

    INTERNALS

******************************************************************************/
{
    time_t ti = *tt;

    ti -= __gmtoffset * 60;

    return gmtime_r (&ti, tm);
} /* localtime_r */
