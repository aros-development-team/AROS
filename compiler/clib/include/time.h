#ifndef _TIME_H_
#define _TIME_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file time.h
    Lang: english
*/
#include <sys/_types.h>
#include <sys/cdefs.h>
#include <sys/arosc.h>

#ifndef __AROS_TIME_T_DECLARED
#define __AROS_TIME_T_DECLARED
typedef __time_t            time_t;
#endif

#ifndef __AROS_CLOCK_T_DECLARED
#define __AROS_CLOCK_T_DECLARED
typedef __clock_t           clock_t;
#endif

#define __need_size_t
#define __need_NULL
#include <stddef.h>

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

#if !defined(_ANSI_SOURCE) && defined(_P1003_1B_VISIBLE)

#ifndef	__AROS_TIMER_T_DECLARED
#define	__AROS_TIMER_T_DECLARED
typedef	__timer_t           timer_t;
#endif

#ifndef	__AROS_CLOCKID_T_DECLARED
#define	__AROS_CLOCKID_T_DECLARED
typedef __clockid_t         clockid_t;
#endif

struct timespec
{
    time_t		tv_sec;		/* seconds */
    long		tv_nsec;	/* nanoseconds */
};

struct itimerspec
{
    struct timespec	it_interval;	/* timer period */
    struct timespec	it_value;	/* timer expiration */
};

#define CLOCK_REALTIME		0
#define TIMER_ABSTIME		1

/* time.h shouldn't include signal.h */
struct sigevent;

#endif /* !_ANSI_SOURCE && _P1003_1B_VISIBLE */

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
void       tzset(void);
#endif

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
char      *asctime_r(const struct tm *, char *);
char      *ctime_r(const time_t *, char *);
struct tm *getdate(const char *);
struct tm *gmtime_r(const time_t *, struct tm *);
struct tm *localtime_r(const time_t *, struct tm *);
char      *strptime(const char *, const char *, struct tm *);
#endif /* !_ANSI_SOURCE && !_POSIX_SOURCE */

#if !defined(_ANSI_SOURCE) && defined(_P1003_1B_VISIBLE)
int        clock_getres(clockid_t, struct timespec *);
int        clock_gettime(clockid_t, struct timespec *);
int        clock_settime(clockid_t, const struct timespec *);
int        nanosleep(const struct timespec *, struct timespec *);
int        timer_create(clockid_t, struct sigevent *, timer_t *);
int        timer_delete(timer_t);
int        timer_gettime(timer_t, struct itimerspec *);
int        timer_getoverrun(timer_t);
int        timer_settime(timer_t, int, const struct itimerspec *,
               struct itimerspec *);
#endif /* !_ANSI_SOURCE && _P1003_1B_VISIBLE */

__END_DECLS

#endif /* _TIME_H_ */
