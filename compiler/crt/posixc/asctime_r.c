/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a broken-down time into a string, reentrant.
*/

#include "__posixc_intbase.h"
#include "__crt_time.h"

/*****************************************************************************

    NAME */
#include <time.h>

        char * asctime_r (

/*  SYNOPSIS */
        const struct tm * tm,
        char * buf)

/*  FUNCTION
        The asctime_r() function converts the broken-down time value tm into
        a string with this format:

            "Wed Jun 30 21:49:08 1993\n"

        It is the reentrant variant of asctime(): the result is stored in the
        caller-supplied buffer rather than in a shared static buffer.

    INPUTS
        tm - The broken down time
        buf - Buffer of at least 26 characters to store the string in

    RESULT
        The pointer passed in buf, containing the converted time (with a
        trailing newline), or NULL if tm does not describe a valid time.

    NOTES

    EXAMPLE
        time_t    tt;
        struct tm tm;
        char      str[26];

        // Get time
        time (&tt);

        // Break time up
        localtime_r (&tt, &tm);

        // Convert to string
        asctime_r (&tm, str);

    BUGS

    SEE ALSO
        time(), ctime_r(), gmtime_r(), localtime_r(), asctime()

    INTERNALS
        The formatting algorithm and the C-locale name tables are shared with
        the ISO C asctime() via the __CRT_ASCTIME() macro in <__crt_time.h>;
        here it formats directly into the caller's buffer (re-entrant) and
        reads the shared name tables from stdc.library's libbase.

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct StdCBase *StdCBase = PosixCBase->PosixCBase.StdCBase;

    if (!__CRT_ASCTIME_VALID(tm))
        return NULL;

    __CRT_ASCTIME(StdCBase, tm, buf);

    return buf;
} /* asctime_r */

