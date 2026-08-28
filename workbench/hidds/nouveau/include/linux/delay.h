/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_DELAY_H_
#define _LINUX_DELAY_H_

#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/sched.h>

void udelay(unsigned long usecs);
void ndelay(unsigned long nsecs);
void mdelay(unsigned long msecs);
void msleep(unsigned int msecs);
unsigned long msleep_interruptible(unsigned int msecs);
void usleep_range(unsigned long min, unsigned long max);
#define usleep_range_state(a, b, s)  usleep_range(a, b)
#define fsleep(us)                   usleep_range(us, us)
#define ssleep(s)                    msleep((s) * 1000)

#endif /* _LINUX_DELAY_H_ */
