#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file sys/time.h
    Lang: english
*/

#include <aros/system.h>
#include <aros/types/timeval_s.h> /* get struct timeval */

/* struct itimerval is used by the interval timers getitimer()/setitimer() */
struct itimerval
{
    struct timeval	it_interval;	    /* timer interval */
    struct timeval	it_value;	    /* current value */
};

/* Which interval timer */
#define ITIMER_REAL	    0	    /* Decrements in real time */
#define ITIMER_VIRTUAL	    1	    /* Decrements in task virtual time */
#define ITIMER_PROF	    2	    /* Decrements in task virtual/system time */

/*
    This structure describes a timezone. Note that most implementations of the
    C library no longer use the timezone information passed to settimeofday().
*/
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

/* Convenience macros for working with timevals */
#define timerclear(tvp)         ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define timerisset(tvp)         ((tvp)->tv_sec || (tvp)->tv_usec)
#define timercmp(tvp, uvp, cmp)                                 \
        (((tvp)->tv_sec == (uvp)->tv_sec) ?                             \
            ((tvp)->tv_usec cmp (uvp)->tv_usec) :                       \
            ((tvp)->tv_sec cmp (uvp)->tv_sec))
#define timeradd(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
                if ((vvp)->tv_usec >= 1000000) {                        \
                        (vvp)->tv_sec++;                                \
                        (vvp)->tv_usec -= 1000000;                      \
                }                                                       \
        } while (0)
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)

__BEGIN_DECLS

/* clib functions */
int getitimer(int which, struct itimerval *);
int setitimer(int which, const struct itimerval *, struct itimerval *);
int gettimeofday(struct timeval * tv, struct timezone * tz);
int settimeofday(const struct timeval * tv, const struct timezone * tz);
int utimes(const char *file, const struct timeval tvp[2]);

__END_DECLS

/*
    SUSv2 says that select() is defined here. BSD however defines it in
    unistd.h. So does AROS, as that makes more sense.
*/

#endif /* _SYS_TIME_H_ */
