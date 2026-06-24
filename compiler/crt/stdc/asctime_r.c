/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a time into a string, reentrant.
*/

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stddef.h>
#include <time.h>

        char * asctime_r (

/*  SYNOPSIS */
        const struct tm * tm,
        char * buf)

/*  FUNCTION
        The asctime_r() function converts the broken-down time value tm
        into a string with this format:

            "Wed Jun 30 21:49:08 1993\n"

    INPUTS
        tm - The broken down time
        buf - Buffer of at least 26 characters to store the string in

    RESULT
        The pointer passed in buf, containing the converted time. Note that
        there is a newline at the end of the buffer.

    NOTES

    EXAMPLE
        time_t    tt;
        struct tm tm;
        char      str[26];

        // Get time
        time (&tt);

        // Break time up
        localtime (&tt, &tm);

        // Convert to string
        asctime (&tm, str);

    BUGS

    SEE ALSO
        time(), ctime_r(), gmtime_r(), localtime_r()

    INTERNALS

******************************************************************************/
{
    /* asctime() always uses the English C-locale names, independent of the
       active locale (C99 7.23.3.1). */
    static const char wday_name[7][3] =
    {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char mon_name[12][3] =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    if (tm == NULL || buf == NULL
        || tm->tm_wday < 0 || tm->tm_wday > 6
        || tm->tm_mon  < 0 || tm->tm_mon  > 11)
    {
        return NULL;
    }

    /* The fixed 26-byte representation: "Www Mmm dd hh:mm:ss yyyy\n".  The
       %.3s precision bounds the read of the (non-NUL-terminated) name arrays,
       and snprintf() guarantees the 26-byte buffer is never overrun. */
    snprintf(buf, 26, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
             wday_name[tm->tm_wday], mon_name[tm->tm_mon],
             tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
             1900 + tm->tm_year);

    return buf;
} /* asctime_r */
