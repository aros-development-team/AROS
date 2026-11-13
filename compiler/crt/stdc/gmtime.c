/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a time into UTC.
*/

#include "__stdc_intbase.h"
#include "__crt_time.h"

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
        time_t      tt;
        struct tm * tm;

        // Get the time
        time (&tt);

        // and convert it
        tm = gmtime (&tt);

    BUGS

    SEE ALSO
        time(), ctime(), asctime(), localtime(), gmtime_r()

    INTERNALS
        The conversion algorithm is shared with the re-entrant gmtime_r() via
        the __CRT_GMTIME() macro in <__crt_time.h>; here it targets the shared
        per-base buffer.

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct tm          *tm = &StdCBase->tmbuffer;

    __CRT_GMTIME(&StdCBase->StdCBase, tt, tm);

    return tm;
} /* gmtime */

