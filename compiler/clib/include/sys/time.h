#ifndef _SYS_TIME_H
#define _SYS_TIME_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file sys/time.h
    Lang: english
*/

/* This structure describes a specific time. See also <devices/timer.h>. */
#ifndef DEVICES_TIMER_H
struct timeval
{
    long tv_sec;  /* Seconds passed. */
    long tv_usec; /* Microseconds passed in the current second. */
};
#endif /* DEVICES_TIMER_H */

/* This structure describes a timezone. */
struct timezone
{
    int  tz_minuteswest; /* Minutes west of Greenwich. */
    int  tz_dsttime;     /* Type of dst correction (see below). */
};

/* Daylight saving time styles. (tz_dsttime) */
#define DST_NONE    0  /* No special style. */
#define DST_USA     1  /* USA style. */
#define DST_AUST    2  /* Australian style. */
#define DST_WET     3  /* Western European style. */
#define DST_MET     4  /* Middle European style. */
#define DST_EET     5  /* Eastern European style. */
#define DST_CAN     6  /* Canadian style. */
#define DST_GB      7  /* Great British and Irish style. */
#define DST_RUM     8  /* Rumanian style. */
#define DST_TUR     9  /* Turkish style. */
#define DST_AUSTALT 10 /* Alternate Australian style. */

/* clib functions */
int gettimeofday(struct timeval * tv, struct timezone * tz);
int settimeofday(const struct timeval * tv, const struct timezone * tz);
int utimes(const char *file, struct timeval tvp[2]);

#endif /* _SYS_TIME_H */
