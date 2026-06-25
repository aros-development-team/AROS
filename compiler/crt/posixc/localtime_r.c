/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a time into a broken-down local time, reentrant.
*/

#include "__posixc_intbase.h"
#include "__crt_time.h"

/*****************************************************************************

    NAME */
#include <time.h>

        struct tm * localtime_r (

/*  SYNOPSIS */
        const time_t * tt,
        struct tm * tm)

/*  FUNCTION
        The localtime_r() function converts the calendar time tt (assumed to
        be UTC) to a broken-down time representation, expressed in the user's
        local timezone.  It is the reentrant variant of localtime(): the
        result is stored in the caller-supplied tm rather than in a shared
        static buffer.

        See localtime() for a description of the members of the tm structure.

    INPUTS
        tt - A time in seconds from the 1. Jan 1970
        tm - A struct tm to store the result in

    RESULT
        The pointer passed in tm.

    NOTES

    EXAMPLE
        time_t    tt;
        struct tm tm;

        // Get time
        time (&tt);

        // Break time up
        localtime_r (&tt, &tm);

    BUGS

    SEE ALSO
        time(), ctime_r(), asctime_r(), gmtime_r(), localtime()

    INTERNALS
        The conversion algorithm is shared with the ISO C localtime() via the
        __CRT_LOCALTIME() macro in <__crt_time.h>; here it converts directly
        into the caller's buffer (re-entrant) and reads the shared month table
        from stdc.library's libbase.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct StdCBase *StdCBase = PosixCBase->PosixCBase.StdCBase;

    __CRT_LOCALTIME(StdCBase, tt, tm);

    return tm;
} /* localtime_r */

