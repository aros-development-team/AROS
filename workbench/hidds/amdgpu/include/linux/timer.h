/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_TIMER_H_
#define _LINUX_TIMER_H_

#include <linux/ktime.h>
#include <linux/list.h>
#include <devices/timer.h>
#include <linux/stddef.h>
#include <linux/debugobjects.h>
#include <linux/stringify.h>

/* kernel timers are delivered through the delayed-work timer process */
struct timer_list {
    void (*function)(struct timer_list *);
    unsigned long expires;
    struct timerequest req;
    BOOL pending;
    struct MinNode node;
};
#define from_timer(var, callback_timer, timer_fieldname) container_of(callback_timer, typeof(*var), timer_fieldname)
#define timer_container_of(var, callback_timer, timer_fieldname) from_timer(var, callback_timer, timer_fieldname)
#define DEFINE_TIMER(name, fn) struct timer_list name = { .function = fn }
#define TIMER_DEFERRABLE 0
#define TIMER_IRQSAFE 0
void timer_setup(struct timer_list *t, void (*func)(struct timer_list *), unsigned int flags);
int  mod_timer(struct timer_list *t, unsigned long expires);
void add_timer(struct timer_list *t);
int  del_timer(struct timer_list *t);
int  del_timer_sync(struct timer_list *t);
int  timer_pending(const struct timer_list *t);
#define timer_delete(t)             del_timer(t)
#define timer_delete_sync(t)        del_timer_sync(t)
#define timer_shutdown_sync(t)      del_timer_sync(t)
#define timer_shutdown(t)           del_timer(t)
#define try_to_del_timer_sync(t)    del_timer(t)
#define round_jiffies_up(j)         (j)
#define round_jiffies_up_relative(j) (j)
#define round_jiffies(j)            (j)
#define round_jiffies_relative(j)   (j)

#endif /* _LINUX_TIMER_H_ */
