#ifndef _TIME_H
#define _TIME_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file time.h
    Lang: english
*/

#if !defined(__typedef_time_t) && !defined(_TIME_T)
#   define __typedef_time_t
#   define _TIME_T
    typedef long time_t;
#endif

#if !defined(__typedef_clock_t) && !defined(_CLOCK_T)
#   define __typedef_clock_t
    typedef long clock_t;
#endif

#if !defined(_SIZE_T) && !defined(__typedef_size_t)
#   define __typedef_size_t
#   define _SIZE_T
    /* Must be int and not long. Otherwise gcc will complain */
    typedef unsigned int size_t;
#endif

#ifndef NULL
#   ifdef __cplusplus
#	define NULL    0
#   else
#	define NULL    ((void *) 0)
#   endif
#endif

#define CLOCKS_PER_SEC 50

struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

extern clock_t clock (void);
extern time_t  time (time_t * tp);
extern double  difftime (time_t time2, time_t time1);
extern time_t  mktime (struct tm * tp);

extern char * asctime (const struct tm * tp);
extern char * ctime (const time_t * tp);
extern size_t strftime (char * s, size_t smax,
			const char * fmt, const struct tm * tp);
extern char * strptime (char * s, const char * fmt, struct tm * tm);

extern void tzset (void);

extern struct tm * gmtime    (const time_t * tp);
extern struct tm * localtime (const time_t * tp);

#endif /* _TIME_H */
