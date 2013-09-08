/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into a string.
*/

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <time.h>

	char * asctime (

/*  SYNOPSIS */
	const struct tm * tm)

/*  FUNCTION
	The asctime() function converts the broken-down time value tm
	into a string.

        See asctime_r() for details.

    INPUTS
	tm - The broken down time

    RESULT
	A statically allocated buffer with the converted time. Note that
	the contents of the buffer might get lost with the call of any of the
        date and time functions.

    NOTES
        The returned string is buffered per stdc.library base.

    EXAMPLE
	time_t	    tt;
	struct tm * tm;
	char	  * str;

	// Get time
	time (&tt);

	// Break time up
	tm = localtime (&tt);

	// Convert to string
	str = asctime (tm);

    BUGS

    SEE ALSO
	time(), ctime(), gmtime(), localtime()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();

    strftime (StdCBase->timebuffer, 26, "%C\n", tm);

    return StdCBase->timebuffer;
} /* asctime */
