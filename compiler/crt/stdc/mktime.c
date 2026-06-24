/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Convert a broken-down time into calendar time.
*/


static char monthtable[] =
{
 /* JanFebMarAprMayJunJulAugSepOktNov */
    31,28,31,30,31,30,31,31,30,31,30
};

/* Number of days in month 'mon' (0-11) of the Gregorian year 'year'
   (i.e. tm_year + 1900), accounting for leap years. */
static int __mktime_monthdays(int year, int mon)
{
    static const int mdays[12] =
        { 31,28,31,30,31,30,31,31,30,31,30,31 };

    if (mon == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
        return 29;

    return mdays[mon];
}

/*****************************************************************************

    NAME */
#include <time.h>

        time_t mktime (

/*  SYNOPSIS */
        struct tm * utim)

/*  FUNCTION
        The mktime() function converts the broken-down time utim to
        calendar time representation.

    INPUTS
        utim - The broken-down time to convert

    RESULT
        The converted calendar time

    NOTES

    EXAMPLE
        time_t      tt;
        struct tm * tm;

        //Computation which results in a tm
        tm = ...

        // and convert it
        tt = mktime (tm);

    BUGS
        At the moment sanity check is not performed nor a normalization on the
        structure is done

    SEE ALSO
        time(), ctime(), asctime(), localtime(), gmtime()

    INTERNALS
        Rules for leap-years:

        1. every 4th year is a leap year

        2. every 100th year is none

        3. every 400th is one

        4. 1900 was none, 2000 is one

******************************************************************************/
{
    time_t           tt;
    int              leapyear,
                     days,
                     year,
                     i;

    /* --- Normalise out-of-range members (C99 7.23.2.3). --- */
    if (utim->tm_sec < 0 || utim->tm_sec > 59)
    {
        int q = utim->tm_sec / 60, r = utim->tm_sec % 60;
        if (r < 0) { r += 60; --q; }
        utim->tm_sec = r; utim->tm_min += q;
    }
    if (utim->tm_min < 0 || utim->tm_min > 59)
    {
        int q = utim->tm_min / 60, r = utim->tm_min % 60;
        if (r < 0) { r += 60; --q; }
        utim->tm_min = r; utim->tm_hour += q;
    }
    if (utim->tm_hour < 0 || utim->tm_hour > 23)
    {
        int q = utim->tm_hour / 24, r = utim->tm_hour % 24;
        if (r < 0) { r += 24; --q; }
        utim->tm_hour = r; utim->tm_mday += q;
    }
    /* Normalise the month first so day-of-month carry knows the month length. */
    if (utim->tm_mon < 0 || utim->tm_mon > 11)
    {
        int q = utim->tm_mon / 12, r = utim->tm_mon % 12;
        if (r < 0) { r += 12; --q; }
        utim->tm_mon = r; utim->tm_year += q;
    }
    while (utim->tm_mday < 1)
    {
        if (--utim->tm_mon < 0) { utim->tm_mon = 11; --utim->tm_year; }
        utim->tm_mday += __mktime_monthdays(utim->tm_year + 1900, utim->tm_mon);
    }
    for (;;)
    {
        int dim = __mktime_monthdays(utim->tm_year + 1900, utim->tm_mon);
        if (utim->tm_mday <= dim)
            break;
        utim->tm_mday -= dim;
        if (++utim->tm_mon > 11) { utim->tm_mon = 0; ++utim->tm_year; }
    }

    /* Compute number of days in the years before this year and after 1970.
     * 1972 is the first leapyear
     */
    year = utim->tm_year-1;
    days = 365*(year-69) + (year-68)/4 - year/100 + (year+300)/400;

    /* Add the day of the months before this month */
    for (i=0; i<utim->tm_mon; i++)
    {
        days += monthtable[i];
    }
    
    /* Is this a leapyear ? */
    year = utim->tm_year;
    leapyear = year%4==0 && (year%100!=0 || (year+300)%400==0);
    if (leapyear && utim->tm_mon>1) days++;

    /* Add day in the current month */
    days += utim->tm_mday - 1;

    /* --- Recompute tm_wday / tm_yday and clear DST (C99 7.23.2.3). --- */
    /* 1970-01-01 was a Thursday, so tm_wday == 4 at day 0. */
    utim->tm_wday = (int)(((days % 7) + 4) % 7);
    if (utim->tm_wday < 0)
        utim->tm_wday += 7;

    {
        int yday = utim->tm_mday - 1;
        for (i=0; i<utim->tm_mon; i++)
            yday += monthtable[i];
        if (leapyear && utim->tm_mon > 1)
            yday++;
        utim->tm_yday = yday;
    }

    utim->tm_isdst = 0;

    tt = ( (days*24+utim->tm_hour)*60 + utim->tm_min )*60 + utim->tm_sec;

    return tt;
} /* mktime */
