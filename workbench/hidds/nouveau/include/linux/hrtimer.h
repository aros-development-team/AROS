/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_HRTIMER_H_
#define _LINUX_HRTIMER_H_

#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/init.h>
#include <linux/percpu.h>

enum hrtimer_mode {
    HRTIMER_MODE_ABS = 0x00,
    HRTIMER_MODE_REL = 0x01,
    HRTIMER_MODE_PINNED = 0x02,
    HRTIMER_MODE_SOFT = 0x04,
    HRTIMER_MODE_HARD = 0x08,
    HRTIMER_MODE_ABS_PINNED = HRTIMER_MODE_ABS | HRTIMER_MODE_PINNED,
    HRTIMER_MODE_REL_PINNED = HRTIMER_MODE_REL | HRTIMER_MODE_PINNED,
};
enum hrtimer_restart {
    HRTIMER_NORESTART,
    HRTIMER_RESTART,
};
struct hrtimer {
    enum hrtimer_restart (*function)(struct hrtimer *);
    struct timer_list timer;
    ktime_t expires;
};
#define CLOCK_MONOTONIC     1
#define CLOCK_REALTIME      0
void hrtimer_init(struct hrtimer *timer, int which_clock, enum hrtimer_mode mode);
void hrtimer_setup(struct hrtimer *timer, enum hrtimer_restart (*function)(struct hrtimer *), int clock, enum hrtimer_mode mode);
void hrtimer_start(struct hrtimer *timer, ktime_t tim, const enum hrtimer_mode mode);
int  hrtimer_cancel(struct hrtimer *timer);
int  hrtimer_try_to_cancel(struct hrtimer *timer);
#define hrtimer_start_range_ns(t, tim, delta, mode) hrtimer_start(t, tim, mode)
#define hrtimer_forward_now(t, interval)   (0)
#define hrtimer_get_expires(t)             ((t)->expires)
#define hrtimer_active(t)                  ((t)->timer.pending)
#define hrtimer_is_queued(t)               ((t)->timer.pending)

#endif /* _LINUX_HRTIMER_H_ */
