/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a time into a string.
*/

#include "__stdc_intbase.h"
#include "__crt_time.h"

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
        time_t      tt;
        struct tm * tm;
        char      * str;

        // Get time
        time (&tt);

        // Break time up
        tm = localtime (&tt);

        // Convert to string
        str = asctime (tm);

    BUGS

    SEE ALSO
        time(), ctime(), gmtime(), localtime(), asctime_r()

    INTERNALS
        The formatting algorithm and the C-locale name tables are shared with
        the re-entrant asctime_r() via the __CRT_ASCTIME() macro in
        <__crt_time.h>; here it targets the shared per-base buffer.

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    char               *buf = StdCBase->timebuffer;

    if (!__CRT_ASCTIME_VALID(tm))
        return NULL;

    __CRT_ASCTIME(&StdCBase->StdCBase, tm, buf);

    return buf;
} /* asctime */

