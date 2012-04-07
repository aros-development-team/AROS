#ifndef _TIME_H_
#define _TIME_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C99 & POSIX.1-2008 header file time.h
*/
#include <aros/system.h>


/* C99 */
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

/* Reentrant versions will be put as extension to C99 in arosstdc.library */
char *asctime_r(const struct tm *, char *);
char *ctime_r(const time_t *, char *);
struct tm *gmtime_r(const time_t *, struct tm *);
struct tm *localtime_r(const time_t *, struct tm *);

__END_DECLS


/* POSIX.1-2008 */
#include <sys/arosc.h>

/* TODO: CLOCKS_PER_SEC is supposed to be 1000000 on SUSv2 platforms apparently */

#include <aros/types/clockid_t.h>
#include <aros/types/timer_t.h>
#include <aros/types/pid_t.h>
#include <aros/types/timespec_s.h>
#include <aros/types/itimerspec_s.h>

struct sigevent;

#define CLOCK_MONOTONIC		0
#define CLOCK_PROCESS_CPUTIME_ID 1
#define CLOCK_REALTIME		2
#define CLOCK_THREAD_CPUTIME_ID 3

#define TIMER_ABSTIME		0x01

/* NOTIMPL
   daylight
   timezone
   tzname
*/

__BEGIN_DECLS

/* NOTIMPL int clock_getcpuclockid(pid_t, clockid_t *); */
/* NOTIMPL int clock_getres(clockid_t, struct timespec *); */
/* NOTIMPL int clock_gettime(clockid_t, struct timespec *); */
/* NOTIMPL int clock_nanosleep(clockid_t, int, const struct timespec *, struct timespec *); */
/* NOTIMPL int clock_settime(clockid_t, const struct timespec *); */
/* NOTIMPL struct tm *getdate(const char *); */
int nanosleep(const struct timespec *, struct timespec *);
/* NOTIMPL size_t strftime_l(char *restrict, size_t, const char *restrict, const struct tm *restrict, locale_t); */
char *strptime(const char *, const char *, struct tm *);
/* NOTIMPL int timer_create(clockid_t, struct sigevent *, timer_t *); */
/* NOTIMPL int timer_delete(timer_t); */
/* NOTIMPL int timer_getoverrun(timer_t); */
/* NOTIMPL int timer_gettime(timer_t, struct itimerspec *); */
/* NOTIMPL int timer_settime(timer_t, int, const struct itimerspec *,
               struct itimerspec *); */
/* NOTIMPL void tzset(void); */

__END_DECLS

#endif /* _TIME_H_ */
