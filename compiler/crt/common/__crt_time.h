/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Shared broken-down-time conversion algorithms.

    The algorithms are defined once here, as macros, and are used by both the
    (buffer-sharing) ISO C implementations in stdc.library and the re-entrant
    POSIX _r implementations in posixc.library, so the conversion logic only
    ever exists in a single place.

    The constant tables the algorithms reference live in the public StdCBase
    (see <libraries/stdc.h>); the caller passes that StdCBase together with the
    destination struct/buffer, so the very same algorithm serves both the
    static-buffer (unsafe) and caller-buffer (re-entrant) variants.
*/
#ifndef _CRT_COMMON_CRT_TIME_H
#define _CRT_COMMON_CRT_TIME_H

#include <stdio.h>
#include <time.h>

#include <libraries/stdc.h>

/* Internal stdc.library export returning the locale GMT offset in minutes. */
int __stdc_gmtoffset(void);

/* Fill the broken-down UTC time _tm (struct tm *) from _tt (const time_t *),
   using the days-per-month table held in _base (struct StdCBase *).

   Rules for leap-years: every 4th year is a leap year; every 100th is not;
   every 400th is; 1900 was not, 2000 is. */
#define __CRT_GMTIME(_base, _tt, _tm)                                          \
do {                                                                           \
    const char  *__crt_mdays = (_base)->__time_monthdays;                      \
    signed long  __crt_tim = *(_tt);                                           \
    int          __crt_leapday = 0, __crt_leapyear = 0, __crt_i;               \
                                                                               \
    (_tm)->tm_sec = __crt_tim % 60; __crt_tim /= 60;                           \
    (_tm)->tm_min = __crt_tim % 60; __crt_tim /= 60;                           \
                                                                               \
    /* 719162: days between 1.1.1 and 1.1.1970. */                            \
    (_tm)->tm_hour = __crt_tim % 24;                                           \
    __crt_tim = __crt_tim / 24 + 719162;                                       \
                                                                               \
    (_tm)->tm_wday = (__crt_tim + 1) % 7;                                      \
                                                                               \
    /* 146097: days from 1.1.1 to 1.1.401. */                                 \
    (_tm)->tm_year = __crt_tim / 146097 * 400 - 1899;                          \
    __crt_tim %= 146097;                                                       \
                                                                               \
    /* 145731: days from 1.1.1 to 1.1.400. */                                 \
    if (__crt_tim >= 145731) {                                                 \
        __crt_leapyear++;       /* The day is in one of the 400th. */          \
        /* The last of the 4 centuries is 1 day longer. */                    \
        if (__crt_tim == 146096) { __crt_tim--; __crt_leapday++; }             \
    }                                                                          \
                                                                               \
    /* 36524: days from 1.1.1 to 1.1.101. */                                  \
    (_tm)->tm_year += __crt_tim / 36524 * 100;                                 \
    __crt_tim %= 36524;                                                        \
                                                                               \
    /* 36159: days from 1.1.1 to 1.1.100. */                                  \
    if (__crt_tim >= 36159) __crt_leapyear--;                                  \
                                                                               \
    /* 1461: days from 1.1.1 to 1.1.5. */                                     \
    (_tm)->tm_year += __crt_tim / 1461 * 4;                                    \
    __crt_tim %= 1461;                                                         \
                                                                               \
    /* 1095: days from 1.1.1 to 1.1.4. */                                     \
    if (__crt_tim >= 1095) {                                                   \
        __crt_leapyear++;       /* The day is in one of the 4th. */            \
        /* The 4th year is 1 day longer. */                                   \
        if (__crt_tim == 1460) { __crt_tim--; __crt_leapday++; }               \
    }                                                                          \
                                                                               \
    /* 365 days in a normal year. */                                          \
    (_tm)->tm_year += __crt_tim / 365;                                         \
    __crt_tim = __crt_tim % 365 + __crt_leapday;                               \
                                                                               \
    (_tm)->tm_yday = __crt_tim;                                                \
    if (!__crt_leapyear && __crt_tim >= 31 + 28) __crt_tim++; /* 29-Feb. */    \
                                                                               \
    for (__crt_i = 0; __crt_i < 11; __crt_i++) {                              \
        if (__crt_tim < __crt_mdays[__crt_i]) break;                           \
        __crt_tim -= __crt_mdays[__crt_i];                                     \
    }                                                                          \
                                                                               \
    (_tm)->tm_mon = __crt_i;                                                   \
    (_tm)->tm_mday = __crt_tim + 1;                                            \
    (_tm)->tm_isdst = 0;                                                       \
    (_tm)->tm_gmtoff = 0;                                                      \
    (_tm)->tm_zone = NULL;                                                     \
} while (0)

/* Fill the broken-down local time _tm from _tt, applying the locale GMT offset.
   AROS currently has no daylight-saving information, so tm_isdst is always 0
   and tm_gmtoff is never corrected for DST. */
#define __CRT_LOCALTIME(_base, _tt, _tm)                                       \
do {                                                                           \
    int     __crt_off = __stdc_gmtoffset() * 60;                              \
    time_t  __crt_local = (time_t)(*(_tt)) - __crt_off;                        \
    __CRT_GMTIME((_base), &__crt_local, (_tm));                                \
    (_tm)->tm_isdst = 0;                                                       \
    (_tm)->tm_gmtoff = -__crt_off;                                             \
} while (0)

/* True if _tm has an in-range weekday and month for asctime() formatting. */
#define __CRT_ASCTIME_VALID(_tm)                                               \
    (!((_tm) == NULL || (_tm)->tm_wday < 0 || (_tm)->tm_wday > 6 ||            \
       (_tm)->tm_mon < 0 || (_tm)->tm_mon > 11))

/* Format _tm into _buf (>= 26 bytes) in the fixed 26-byte representation
   "Www Mmm dd hh:mm:ss yyyy\n", using the English C-locale name tables held in
   _base (independent of the active locale, C99 7.23.3.1).  The %.3s precision
   bounds the read of the non-NUL-terminated name arrays, and snprintf()
   guarantees the buffer is never overrun. */
#define __CRT_ASCTIME(_base, _tm, _buf)                                        \
    snprintf((_buf), 26, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",                   \
             (_base)->__time_wday_name[(_tm)->tm_wday],                        \
             (_base)->__time_mon_name[(_tm)->tm_mon],                          \
             (_tm)->tm_mday, (_tm)->tm_hour, (_tm)->tm_min, (_tm)->tm_sec,     \
             1900 + (_tm)->tm_year)

#endif /* _CRT_COMMON_CRT_TIME_H */
