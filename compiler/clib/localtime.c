/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into a string.
*/

extern long __gmtoffset;

/*****************************************************************************

    NAME */
#include <time.h>

	struct tm * localtime (

/*  SYNOPSIS */
	const time_t * tt)

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

    RESULT
	A statically allocated buffer with the broken up time. Note that
	the contents of the buffer might get lost with the call of any of
	the date and time functions.

    NOTES
        This function must not be used in a shared library or
        in a threaded application.

    EXAMPLE
	time_t	    tt;
	struct tm * tm;

	// Get time
	time (&tt);

	// Break time up
	tm = localtime (&tt);

    BUGS

    SEE ALSO
	time(), ctime(), asctime()

    INTERNALS

******************************************************************************/
{
    time_t ti = *tt;

    ti -= __gmtoffset * 60;

    return gmtime (&ti);
} /* localtime */
