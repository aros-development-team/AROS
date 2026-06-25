/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a time into a string, reentrant.
*/

/*****************************************************************************

    NAME */
#include <time.h>

        char * ctime_r (

/*  SYNOPSIS */
        const time_t * tt,
        char * buf)

/*  FUNCTION
        The ctime_r() function converts the time value tt into a string with
        this format:

            "Wed Jun 30 21:49:08 1993\n"

        It is the reentrant variant of ctime(): the result is stored in the
        caller-supplied buffer rather than in a shared static buffer.  It is
        equivalent to asctime_r(localtime_r(tt, &tm), buf).

    INPUTS
        tt - Convert this time.
        buf - Buffer of at least 26 characters to store the string in

    RESULT
        The pointer passed in buf, containing the converted time (with a
        trailing newline), or NULL on failure.

    NOTES

    EXAMPLE
        time_t tt;
        char   str[26];

        // Get time
        time (&tt);

        // Convert to string
        ctime_r (&tt, str);

    BUGS

    SEE ALSO
        time(), asctime_r(), gmtime_r(), localtime_r(), ctime()

    INTERNALS

******************************************************************************/
{
    struct tm tm;

    return asctime_r (localtime_r (tt, &tm), buf);
} /* ctime_r */
