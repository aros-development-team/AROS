#ifndef _POSIXC_TIME_H_
#define _POSIXC_TIME_H_

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 header file time.h
*/

/* C99 */
#include <aros/stdc/time.h>

/* TODO: CLOCKS_PER_SEC is supposed to be 1000000 on SUSv2 platforms apparently */

#include <aros/posixc/locale.h>

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
   getdate_err
*/

/* POSIX timezone variables, set by tzset() (implemented in posixc.library). */
extern int   daylight;          /* nonzero if a daylight-saving zone is set  */
extern long  timezone;          /* seconds West of UTC                       */
extern char *tzname[2];          /* { standard, daylight } abbreviations      */


__BEGIN_DECLS

/* Reentrant (_r) variants of the ISO C time functions are POSIX additions
   implemented in posixc.library. */
char *asctime_r(const struct tm *, char *);
char *ctime_r(const time_t *, char *);
struct tm *gmtime_r(const time_t *, struct tm *);
struct tm *localtime_r(const time_t *, struct tm *);

void tzset(void);

/* NOTIMPL int clock_getcpuclockid(pid_t, clockid_t *); */
/* NOTIMPL int clock_getres(clockid_t, struct timespec *); */
int clock_gettime(clockid_t, struct timespec *);
/* NOTIMPL int clock_nanosleep(clockid_t, int, const struct timespec *, struct timespec *); */
/* NOTIMPL int clock_settime(clockid_t, const struct timespec *); */
/* NOTIMPL struct tm *getdate(const char *); */
int nanosleep(const struct timespec *, struct timespec *);
size_t strftime_l(char *restrict, size_t, const char *restrict, const struct tm *restrict, locale_t);
char *strptime(const char *, const char *, struct tm *);
/* NOTIMPL int timer_create(clockid_t, struct sigevent *, timer_t *); */
/* NOTIMPL int timer_delete(timer_t); */
/* NOTIMPL int timer_getoverrun(timer_t); */
/* NOTIMPL int timer_gettime(timer_t, struct itimerspec *); */
/* NOTIMPL int timer_settime(timer_t, int, const struct itimerspec *,
               struct itimerspec *); */

__END_DECLS

#endif /* _POSIXC_TIME_H_ */
