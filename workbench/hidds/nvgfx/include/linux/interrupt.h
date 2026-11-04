/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_INTERRUPT_H_
#define _LINUX_INTERRUPT_H_

#include <linux/types.h>
#include <linux/irqreturn.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/hrtimer.h>
#include <linux/kref.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/bitmap.h>

typedef irqreturn_t (*irq_handler_t)(int, void *);

#define IRQF_SHARED             0x00000080
#define IRQF_ONESHOT            0x00002000
#define IRQF_NO_THREAD          0x00010000
#define IRQF_TRIGGER_NONE       0x00000000
#define IRQF_TRIGGER_RISING     0x00000001
#define IRQF_TRIGGER_FALLING    0x00000002
#define IRQF_TRIGGER_HIGH       0x00000004
#define IRQF_TRIGGER_LOW        0x00000008
#define IRQF_NOBALANCING        0x00000800
#define IRQF_NO_AUTOEN          0x00080000
#define IRQF_NO_SUSPEND         0x00004000

/*
 * A threaded handler runs on the work queue: the hard handler is called
 * from the interrupt and, if it answers IRQ_WAKE_THREAD, the thread part
 * is queued.
 */
int  request_threaded_irq(unsigned int irq, irq_handler_t handler, irq_handler_t thread_fn, unsigned long flags, const char *name, void *dev);
static inline int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev)
{
    return request_threaded_irq(irq, handler, NULL, flags, name, dev);
}
void *free_irq(unsigned int irq, void *dev_id);
#define devm_request_irq(d, i, h, f, n, dev)            request_irq(i, h, f, n, dev)
#define devm_request_threaded_irq(d, i, h, t, f, n, dev) request_threaded_irq(i, h, t, f, n, dev)
#define devm_free_irq(d, i, dev)                        free_irq(i, dev)
#define disable_irq(i)          do { } while (0)
#define enable_irq(i)           do { } while (0)
#define disable_irq_nosync(i)   do { } while (0)
#define synchronize_irq(i)      do { } while (0)
#define irq_set_affinity_hint(i, m) (0)
#define irq_get_irq_data(i)     NULL
#define irqd_get_trigger_type(d) 0
#define local_irq_disable_compat() Disable()

/* tasklets become work items */
struct tasklet_struct {
    struct work_struct work;
    void (*func)(unsigned long);
    void (*callback)(struct tasklet_struct *t);
    unsigned long data;
};
void tasklet_setup(struct tasklet_struct *t, void (*callback)(struct tasklet_struct *));
void tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data);
void tasklet_schedule(struct tasklet_struct *t);
void tasklet_hi_schedule(struct tasklet_struct *t);
void tasklet_kill(struct tasklet_struct *t);
#define tasklet_disable(t)      do { } while (0)
#define tasklet_enable(t)       do { } while (0)
#define from_tasklet(v, c, f)   container_of(c, typeof(*v), f)
#define DECLARE_TASKLET(name, func) struct tasklet_struct name = { .callback = func }
#define DECLARE_TASKLET_OLD(name, func) struct tasklet_struct name = { .func = func }

#endif /* _LINUX_INTERRUPT_H_ */
