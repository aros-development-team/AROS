/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a time into UTC, reentrant.
*/

#include "__posixc_intbase.h"
#include "__crt_time.h"

/*****************************************************************************

    NAME */
#include <time.h>

        struct tm * gmtime_r (

/*  SYNOPSIS */
        const time_t * tt,
        struct tm * tm)

/*  FUNCTION
        The gmtime_r() function converts the calendar time tt (assumed to be
        UTC) to a broken-down time representation, expressed in Coordinated
        Universal Time (UTC).  It is the reentrant variant of gmtime(): the
        result is stored in the caller-supplied tm rather than in a shared
        static buffer.

    INPUTS
        tt - The time to convert
        tm - A struct tm to store the result in

    RESULT
        The pointer passed in tm, containing the broken-down time in
        Coordinated Universal Time (UTC).

    NOTES

    EXAMPLE
        time_t    tt;
        struct tm tm;

        // Get the time
        time (&tt);

        // and convert it
        gmtime_r (&tt, &tm);

    BUGS

    SEE ALSO
        time(), ctime_r(), asctime_r(), localtime_r(), gmtime()

    INTERNALS
        The conversion algorithm is shared with the ISO C gmtime() via the
        __CRT_GMTIME() macro in <__crt_time.h>; here it converts directly into
        the caller's buffer (re-entrant) and reads the shared month table from
        stdc.library's libbase.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct StdCBase *StdCBase = PosixCBase->PosixCBase.StdCBase;

    __CRT_GMTIME(StdCBase, tt, tm);

    return tm;
} /* gmtime_r */

