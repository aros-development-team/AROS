/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into UTC.
*/

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <time.h>

	struct tm * gmtime (

/*  SYNOPSIS */
	const time_t * tt)

/*  FUNCTION
	The gmtime() function converts the calendar time tt to
	broken-down time representation, expressed in Coordinated Universal
	Time (UTC).

        See gmtime_r() for details.

    INPUTS
	tt - The time to convert

    RESULT
	A statically allocated buffer with the broken down time in Coordinated
        Universal Time (UTC). Note that the contents of the buffer might get
        lost with the call of any of the date and time functions.

    NOTES
        Resulting tm struct is buffered per stdc.library and shared
        with localtime().

    EXAMPLE
	time_t	    tt;
	struct tm * tm;

	// Get the time
	time (&tt);

	// and convert it
	tm = gmtime (&tt);

    BUGS

    SEE ALSO
	time(), ctime(), asctime(), localtime()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    extern struct tm * gmtime_r (const time_t * tt, struct tm * tm);

    return gmtime_r(tt, &StdCBase->tmbuffer);
} /* gmtime */
