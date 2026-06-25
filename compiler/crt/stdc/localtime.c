/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a time into a string.
*/

#include "__stdc_intbase.h"
#include "__crt_time.h"

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
        time_t      tt;
        struct tm * tm;

        // Get time
        time (&tt);

        // Break time up
        tm = localtime (&tt);

    BUGS

    SEE ALSO
        time(), ctime(), asctime(), gmtime(), localtime_r()

    INTERNALS
        The conversion algorithm is shared with the re-entrant localtime_r()
        via the __CRT_LOCALTIME() macro in <__crt_time.h>; here it targets the
        shared per-base buffer.

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct tm          *tm = &StdCBase->tmbuffer;

    __CRT_LOCALTIME(&StdCBase->StdCBase, tt, tm);

    return tm;
} /* localtime */

