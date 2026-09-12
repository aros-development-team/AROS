/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_IOPOLL_H_
#define _LINUX_IOPOLL_H_

#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/delay.h>

#define read_poll_timeout(op, val, cond, sleep_us, timeout_us, sleep_before_read, args...) \
({                                                                      \
    u64 __timeout_us = (timeout_us);                                    \
    unsigned long __sleep_us = (sleep_us);                              \
    ktime_t __timeout = ktime_add_us(ktime_get(), __timeout_us);        \
    if (sleep_before_read && __sleep_us)                                \
        usleep_range((__sleep_us >> 2) + 1, __sleep_us);                \
    for (;;) {                                                          \
        (val) = op(args);                                               \
        if (cond)                                                       \
            break;                                                      \
        if (__timeout_us && ktime_compare(ktime_get(), __timeout) > 0) { \
            (val) = op(args);                                           \
            break;                                                      \
        }                                                               \
        if (__sleep_us)                                                 \
            usleep_range((__sleep_us >> 2) + 1, __sleep_us);            \
        cpu_relax();                                                    \
    }                                                                   \
    (cond) ? 0 : -ETIMEDOUT;                                            \
})
#define read_poll_timeout_atomic(op, val, cond, delay_us, timeout_us, delay_before_read, args...) \
({                                                                      \
    u64 __timeout_us = (timeout_us);                                    \
    unsigned long __delay_us = (delay_us);                              \
    ktime_t __timeout = ktime_add_us(ktime_get(), __timeout_us);        \
    if (delay_before_read && __delay_us)                                \
        udelay(__delay_us);                                             \
    for (;;) {                                                          \
        (val) = op(args);                                               \
        if (cond)                                                       \
            break;                                                      \
        if (__timeout_us && ktime_compare(ktime_get(), __timeout) > 0) { \
            (val) = op(args);                                           \
            break;                                                      \
        }                                                               \
        if (__delay_us)                                                 \
            udelay(__delay_us);                                         \
        cpu_relax();                                                    \
    }                                                                   \
    (cond) ? 0 : -ETIMEDOUT;                                            \
})
#define readx_poll_timeout(op, addr, val, cond, sleep_us, timeout_us) read_poll_timeout(op, val, cond, sleep_us, timeout_us, false, addr)
#define readx_poll_timeout_atomic(op, addr, val, cond, delay_us, timeout_us) read_poll_timeout_atomic(op, val, cond, delay_us, timeout_us, false, addr)
#define readb_poll_timeout(addr, val, cond, delay_us, timeout_us) readx_poll_timeout(readb, addr, val, cond, delay_us, timeout_us)
#define readw_poll_timeout(addr, val, cond, delay_us, timeout_us) readx_poll_timeout(readw, addr, val, cond, delay_us, timeout_us)
#define readl_poll_timeout(addr, val, cond, delay_us, timeout_us) readx_poll_timeout(readl, addr, val, cond, delay_us, timeout_us)
#define readq_poll_timeout(addr, val, cond, delay_us, timeout_us) readx_poll_timeout(readq, addr, val, cond, delay_us, timeout_us)
#define readl_poll_timeout_atomic(addr, val, cond, delay_us, timeout_us) readx_poll_timeout_atomic(readl, addr, val, cond, delay_us, timeout_us)

#endif /* _LINUX_IOPOLL_H_ */
