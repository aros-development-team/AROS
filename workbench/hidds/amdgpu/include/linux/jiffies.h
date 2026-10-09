/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_JIFFIES_H_
#define _LINUX_JIFFIES_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/time.h>
#include <linux/timex.h>

/* one jiffy is a millisecond; the clock is timer.device's system time */
#define HZ                      1000
#define MAX_JIFFY_OFFSET        ((LONG_MAX >> 1) - 1)
#define MAX_SCHEDULE_TIMEOUT    LONG_MAX
#define INITIAL_JIFFIES         0

unsigned long compat_jiffies(void);
u64 compat_jiffies64(void);
#define jiffies                 compat_jiffies()
#define jiffies_64              compat_jiffies64()
#define get_jiffies_64()        compat_jiffies64()

#define time_after(a, b)        ((long)((b) - (a)) < 0)
#define time_before(a, b)       time_after(b, a)
#define time_after_eq(a, b)     ((long)((a) - (b)) >= 0)
#define time_before_eq(a, b)    time_after_eq(b, a)
#define time_in_range(a, b, c)  (time_after_eq(a, b) && time_before_eq(a, c))
#define time_after64(a, b)      ((s64)((b) - (a)) < 0)
#define time_before64(a, b)     time_after64(b, a)
#define time_after_eq64(a, b)   ((s64)((a) - (b)) >= 0)
#define time_before_eq64(a, b)  time_after_eq64(b, a)
#define time_is_before_jiffies(a) time_after(jiffies, a)
#define time_is_after_jiffies(a)  time_before(jiffies, a)
#define time_is_before_eq_jiffies(a) time_after_eq(jiffies, a)
#define time_is_after_eq_jiffies(a)  time_before_eq(jiffies, a)

static inline unsigned int jiffies_to_msecs(const unsigned long j)  { return (unsigned int)j; }
static inline unsigned int jiffies_to_usecs(const unsigned long j)  { return (unsigned int)j * 1000; }
static inline u64 jiffies_to_nsecs(const unsigned long j)           { return (u64)j * 1000000ULL; }
static inline unsigned long msecs_to_jiffies(const unsigned int m)
{
    if (m > (unsigned int)MAX_JIFFY_OFFSET)
        return MAX_JIFFY_OFFSET;
    return m;
}
static inline unsigned long usecs_to_jiffies(const unsigned int u)  { return (u + 999) / 1000; }
static inline unsigned long nsecs_to_jiffies(u64 n)                  { return (unsigned long)((n + 999999ULL) / 1000000ULL); }
static inline u64 nsecs_to_jiffies64(u64 n)                          { return (n + 999999ULL) / 1000000ULL; }
static inline unsigned long secs_to_jiffies(unsigned int s)          { return (unsigned long)s * HZ; }
static inline u64 get_jiffies_64_ns(void)                            { return jiffies_to_nsecs(compat_jiffies()); }

#endif /* _LINUX_JIFFIES_H_ */
