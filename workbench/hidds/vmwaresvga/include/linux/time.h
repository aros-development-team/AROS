/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_TIME_H_
#define _LINUX_TIME_H_

#include <linux/types.h>
#include <linux/math64.h>

#define MSEC_PER_SEC            1000L
#define USEC_PER_MSEC           1000L
#define NSEC_PER_USEC           1000L
#define NSEC_PER_MSEC           1000000L
#define USEC_PER_SEC            1000000L
#define NSEC_PER_SEC            1000000000L
#define PSEC_PER_SEC            1000000000000LL
#define FSEC_PER_SEC            1000000000000000LL
#define KTIME_MAX               ((s64)~((u64)1 << 63))
#define KTIME_MIN               (-KTIME_MAX - 1)
#define KTIME_SEC_MAX           (KTIME_MAX / NSEC_PER_SEC)

struct timespec64 {
    time64_t tv_sec;
    long tv_nsec;
};
struct __kernel_timespec {
    s64 tv_sec;
    s64 tv_nsec;
};
static inline s64 timespec64_to_ns(const struct timespec64 *ts)
{
    return ((s64)ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}
static inline struct timespec64 ns_to_timespec64(s64 nsec)
{
    struct timespec64 ts;
    ts.tv_sec = nsec / NSEC_PER_SEC;
    ts.tv_nsec = nsec % NSEC_PER_SEC;
    if (ts.tv_nsec < 0) { ts.tv_sec--; ts.tv_nsec += NSEC_PER_SEC; }
    return ts;
}
static inline s64 timespec64_sub_ns(struct timespec64 a, struct timespec64 b) { return timespec64_to_ns(&a) - timespec64_to_ns(&b); }
static inline bool timespec64_valid(const struct timespec64 *ts) { return ts->tv_sec >= 0 && ts->tv_nsec >= 0 && ts->tv_nsec < NSEC_PER_SEC; }

#endif /* _LINUX_TIME_H_ */
