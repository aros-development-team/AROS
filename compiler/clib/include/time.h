#ifndef _TIME_H_
#define _TIME_H_

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file time.h
    Lang: english
*/
#include <aros/system.h>
#include <sys/arosc.h>

#include <aros/types/time_t.h>
#include <aros/types/clock_t.h>
#include <aros/types/size_t.h>
#include <aros/types/null.h>

/* XXX: This is supposed to be 1000000 on SUSv2 platforms apparently */
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

#if !defined(_ANSI_SOURCE)

#include <aros/types/timer_t.h>
#include <aros/types/clockid_t.h>
#include <aros/types/timespec_s.h>
#include <aros/types/itimerspec_s.h>

#define CLOCK_REALTIME		0
#define TIMER_ABSTIME		1

/* time.h shouldn't include signal.h */
struct sigevent;

#endif /* !_ANSI_SOURCE */

#define __daylight (__get_arosc_userdata()->acud_daylight)
#define __timezone (__get_arosc_userdata()->acud_timezone)

#define __tzname   (__get_arosc_userdata()->acud_tzname)

__BEGIN_DECLS
char      *asctime(const struct tm *);
clock_t    clock(void);
char      *ctime(const time_t *);
double     difftime(time_t, time_t);
struct tm *gmtime(const time_t *);
struct tm *localtime(const time_t *);
time_t     mktime(struct tm *);
size_t     strftime(char *, size_t, const char *, const struct tm *);
time_t     time(time_t *);

#if !defined(_ANSI_SOURCE)
/* NOTIMPL void       tzset(void); */
#endif

char      *asctime_r(const struct tm *, char *);
char      *ctime_r(const time_t *, char *);
struct tm *gmtime_r(const time_t *, struct tm *);
struct tm *localtime_r(const time_t *, struct tm *);

/* NOTIMPL struct tm *getdate(const char *); */
char      *strptime(const char *, const char *, struct tm *);

/* NOTIMPL int        clock_getres(clockid_t, struct timespec *); */
/* NOTIMPL int        clock_gettime(clockid_t, struct timespec *); */
/* NOTIMPL int        clock_settime(clockid_t, const struct timespec *); */
int        nanosleep(const struct timespec *, struct timespec *);

/* NOTIMPL int        timer_create(clockid_t, struct sigevent *, timer_t *); */
/* NOTIMPL int        timer_delete(timer_t); */
/* NOTIMPL int        timer_gettime(timer_t, struct itimerspec *); */
/* NOTIMPL int        timer_getoverrun(timer_t); */
/* NOTIMPL int        timer_settime(timer_t, int, const struct itimerspec *,
               struct itimerspec *); */

__END_DECLS

#endif /* _TIME_H_ */
