/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_KTIME_H_
#define _LINUX_KTIME_H_

#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/math64.h>

/* ktime_t is nanoseconds; both clocks come from timer.device system time */
ktime_t ktime_get(void);
#define ktime_get_raw()             ktime_get()
#define ktime_get_boottime()        ktime_get()
#define ktime_get_real()            ktime_get()
#define ktime_get_mono_fast_ns()    ((u64)ktime_get())
#define ktime_get_ns()              ((u64)ktime_get())
#define ktime_get_raw_ns()          ((u64)ktime_get())
#define ktime_get_boottime_ns()     ((u64)ktime_get())
#define ktime_get_real_ns()         ((u64)ktime_get())
#define ktime_get_real_seconds()    ((time64_t)(ktime_get() / NSEC_PER_SEC))
#define ktime_get_seconds()         ((time64_t)(ktime_get() / NSEC_PER_SEC))
#define ktime_get_ts64(ts)          (*(ts) = ns_to_timespec64(ktime_get()))
#define ktime_get_real_ts64(ts)     (*(ts) = ns_to_timespec64(ktime_get()))
#define ktime_get_raw_ts64(ts)      (*(ts) = ns_to_timespec64(ktime_get()))
#define ktime_get_boottime_ts64(ts) (*(ts) = ns_to_timespec64(ktime_get()))
#define ktime_get_coarse_real_ts64(ts) (*(ts) = ns_to_timespec64(ktime_get()))
#define ktime_get_snapshot(s)       do { } while (0)
#define ktime_get_boottime_seconds() ktime_get_seconds()

static inline ktime_t ktime_set(const s64 secs, const unsigned long nsecs)
{
    if (secs >= KTIME_SEC_MAX)
        return KTIME_MAX;
    return secs * NSEC_PER_SEC + (s64)nsecs;
}
#define ktime_sub(lhs, rhs)         ((lhs) - (rhs))
#define ktime_add(lhs, rhs)         ((lhs) + (rhs))
#define ktime_add_unsafe(lhs, rhs)  ((u64)(lhs) + (rhs))
#define ktime_add_ns(kt, nsval)     ((kt) + (nsval))
#define ktime_sub_ns(kt, nsval)     ((kt) - (nsval))
#define ktime_add_us(kt, usec)      ktime_add_ns(kt, (usec) * NSEC_PER_USEC)
#define ktime_add_ms(kt, msec)      ktime_add_ns(kt, (msec) * NSEC_PER_MSEC)
#define ktime_sub_us(kt, usec)      ktime_sub_ns(kt, (usec) * NSEC_PER_USEC)
#define ktime_sub_ms(kt, msec)      ktime_sub_ns(kt, (msec) * NSEC_PER_MSEC)
#define ktime_to_ns(kt)             (kt)
#define ns_to_ktime(ns)             ((ktime_t)(ns))
#define ms_to_ktime(ms)             ((ktime_t)(ms) * NSEC_PER_MSEC)
#define us_to_ktime(us)             ((ktime_t)(us) * NSEC_PER_USEC)
#define ktime_to_us(kt)             ((s64)((kt) / NSEC_PER_USEC))
#define ktime_to_ms(kt)             ((s64)((kt) / NSEC_PER_MSEC))
#define ktime_to_timespec64(kt)     ns_to_timespec64(kt)
#define timespec64_to_ktime(ts)     timespec64_to_ns(&(ts))
#define ktime_us_delta(later, earlier)  ktime_to_us(ktime_sub(later, earlier))
#define ktime_ms_delta(later, earlier)  ktime_to_ms(ktime_sub(later, earlier))
#define ktime_compare(a, b)         (((a) < (b)) ? -1 : ((a) > (b)) ? 1 : 0)
#define ktime_after(a, b)           (ktime_compare(a, b) > 0)
#define ktime_before(a, b)          (ktime_compare(a, b) < 0)
#define ktime_equal(a, b)           ((a) == (b))
#define ktime_divns(kt, div)        ((s64)(kt) / (div))
#define ktime_mul(kt, m)            ((kt) * (m))
#define ktime_to_clock_t(kt)        ktime_to_ms(kt)
#define ktime_get_boottime_ts64(ts) (*(ts) = ns_to_timespec64(ktime_get()))
#define ktime_get_ts(ts)            ktime_get_ts64(ts)
#define ktime_get_real_seconds()    ((time64_t)(ktime_get() / NSEC_PER_SEC))
static inline u64 ktime_get_ns_since(ktime_t start) { return (u64)ktime_sub(ktime_get(), start); }
#define do_gettimeofday(tv)         do { } while (0)
#define getrawmonotonic64(ts)       ktime_get_raw_ts64(ts)
#define ktime_get_real_fast_ns()    ((u64)ktime_get())

#endif /* _LINUX_KTIME_H_ */
