/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Convert a time into a string.
*/

#include "__stdc_intbase.h"

/*****************************************************************************

    NAME */
#include <time.h>

	struct tm * localtime (

/*  SYNOPSIS */
	const time_t * tt)

/*  FUNCTION
	Splits the system time in seconds into a structure.

        See localtime_r() for details.

    INPUTS
	tt - A time in seconds from the 1. Jan 1970

    RESULT
	A statically allocated buffer with the broken up time. Note that
	the contents of the buffer might get lost with the call of any of
	the date and time functions.

    NOTES
        Resulting tm struct is buffered per stdc.library and shared
        with gmtime().
        

    EXAMPLE
	time_t	    tt;
	struct tm * tm;

	// Get time
	time (&tt);

	// Break time up
	tm = localtime (&tt);

    BUGS

    SEE ALSO
	time(), ctime(), asctime(), gmtime()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();

    return localtime_r (tt, &StdCBase->tmbuffer);
} /* localtime */
