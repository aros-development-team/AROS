#ifndef _STDC_TIME_H_
#define _STDC_TIME_H_

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file time.h
*/
#include <aros/system.h>

#include <aros/types/time_t.h>
#include <aros/types/clock_t.h>

#include <aros/types/size_t.h>
#include <aros/types/null.h>

#define CLOCKS_PER_SEC 50

struct tm
{
    int         tm_sec;
    int         tm_min;
    int         tm_hour;
    int         tm_mday;
    int         tm_mon;
    int         tm_year;
    int         tm_wday;
    int         tm_yday;
    int         tm_isdst;
    long int    tm_gmtoff;
    const char *tm_zone;
};


__BEGIN_DECLS

/* Time manipulation functions */
clock_t clock(void);
double difftime(time_t, time_t);
time_t mktime(struct tm *);
time_t time(time_t *);

/* Time conversion functions */
char *asctime(const struct tm *);
char *ctime(const time_t *);
struct tm *gmtime(const time_t *);
struct tm *localtime(const time_t *);
size_t     strftime(char *, size_t, const char *, const struct tm *);

/* Reentrant versions are put as extension to C99 in stdc.library */
char *asctime_r(const struct tm *, char *);
char *ctime_r(const time_t *, char *);
struct tm *gmtime_r(const time_t *, struct tm *);
struct tm *localtime_r(const time_t *, struct tm *);

/* AROS extension */
int __arosc_gmtoffset(void);

__END_DECLS

#endif /* _STDC_TIME_H_ */
