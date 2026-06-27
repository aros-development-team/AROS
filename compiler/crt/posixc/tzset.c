/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX function tzset().
*/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#ifndef TZNAME_MAX
#define TZNAME_MAX 6
#endif

/* POSIX global timezone variables.  tzset() is provided through the static
   link library (like getopt()/optarg), so each program owns its own copy of
   these and of the name buffers they point at. */
long  timezone = 0;             /* seconds West of UTC                       */
int   daylight = 0;             /* nonzero if a DST zone name was given      */
static char __tzname_std[TZNAME_MAX + 1] = "UTC";
static char __tzname_dst[TZNAME_MAX + 1] = "UTC";
char *tzname[2] = { __tzname_std, __tzname_dst };

/* Copy a timezone abbreviation from the TZ value into dst (max TZNAME_MAX),
   returning the number of source characters consumed.  A name is either a run
   of alphabetic characters or, in the modern POSIX form, any run enclosed in
   angle brackets ("<...>").  Returns 0 if no name is present. */
static size_t parse_tzname(const char *p, char *dst)
{
    const char *start = p;
    size_t      i = 0;

    if (*p == '<')
    {
        p++;                                    /* skip '<' */
        while (*p && *p != '>')
        {
            if (i < TZNAME_MAX)
                dst[i++] = *p;
            p++;
        }
        if (*p == '>')
            p++;                                /* skip '>' */
    }
    else
    {
        while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))
        {
            if (i < TZNAME_MAX)
                dst[i++] = *p;
            p++;
        }
    }

    dst[i] = '\0';
    return (size_t)(p - start);
}

/* Parse a POSIX TZ offset "[+|-]hh[:mm[:ss]]" starting at p into *off (seconds
   West of UTC, so a leading '-' - meaning East of UTC - yields a negative
   value).  Returns the number of characters consumed. */
static size_t parse_tzoffset(const char *p, long *off)
{
    const char *start = p;
    int         sign = 1;
    long        hh = 0, mm = 0, ss = 0;

    if (*p == '-')
    {
        sign = -1;
        p++;
    }
    else if (*p == '+')
        p++;

    while (*p >= '0' && *p <= '9')
        hh = hh * 10 + (*p++ - '0');
    if (*p == ':')
    {
        p++;
        while (*p >= '0' && *p <= '9')
            mm = mm * 10 + (*p++ - '0');
        if (*p == ':')
        {
            p++;
            while (*p >= '0' && *p <= '9')
                ss = ss * 10 + (*p++ - '0');
        }
    }

    *off = sign * (hh * 3600 + mm * 60 + ss);
    return (size_t)(p - start);
}

/*****************************************************************************

    NAME */
#include <time.h>

        void tzset (

/*  SYNOPSIS */
        void)

/*  FUNCTION
        Initialise the timezone conversion information used by localtime() and
        related functions from the TZ environment variable, and set the global
        variables timezone, daylight and tzname accordingly.

    INPUTS
        None.

    RESULT
        None.  The external variables are set as follows:

        timezone - The difference, in seconds, between Coordinated Universal
                   Time (UTC) and local standard time (positive West of UTC).

        daylight - Non-zero if the TZ value names a daylight-saving timezone.

        tzname   - tzname[0] is the abbreviation of the standard timezone,
                   tzname[1] that of the daylight-saving timezone.

    NOTES
        Only the POSIX "std offset[dst[offset]][,rule]" form of TZ is
        recognised, and the optional daylight-saving offset and transition
        rules are not applied (AROS keeps no timezone-rule database, so
        daylight-saving time is never actually entered).  When TZ is unset the
        offset is taken from the system locale and the abbreviations default
        to "UTC".

    EXAMPLE
        setenv("TZ", "EST5EDT", 1);
        tzset();
        // timezone == 5*3600, daylight != 0, tzname[0] == "EST"

    BUGS

    SEE ALSO
        localtime(), gmtime(), ctime(), strftime(), getenv()

    INTERNALS
        The locale fall-back uses stdc.library's __stdc_gmtoffset(), the same
        source localtime() and gettimeofday() use, so all three agree on the
        offset.

******************************************************************************/
{
    const char *tz = getenv("TZ");
    char        std[TZNAME_MAX + 1]  = "UTC";
    char        dstn[TZNAME_MAX + 1] = "UTC";
    long        off = 0;
    int         dst = 0;

    if (tz == NULL)
    {
        /* TZ unset: no zone database, so take the offset from the locale and
           keep the generic "UTC" abbreviations. */
        off = (long)__stdc_gmtoffset() * 60;
    }
    else if (*tz != '\0')
    {
        const char *p = tz;

        p += parse_tzname(p, std);
        p += parse_tzoffset(p, &off);

        /* A daylight-saving zone name (if any) follows the std offset. */
        if (parse_tzname(p, dstn) > 0)
            dst = 1;
        else
            memcpy(dstn, std, sizeof(std));
    }
    /* else: empty TZ ("") means UTC0 - the "UTC"/0 defaults already apply. */

    timezone = off;
    daylight = dst;

    strncpy(__tzname_std, std, TZNAME_MAX);
    __tzname_std[TZNAME_MAX] = '\0';
    strncpy(__tzname_dst, dstn, TZNAME_MAX);
    __tzname_dst[TZNAME_MAX] = '\0';

    tzname[0] = __tzname_std;
    tzname[1] = __tzname_dst;
} /* tzset */
