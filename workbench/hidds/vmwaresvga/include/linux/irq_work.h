/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_IRQ_WORK_H_
#define _LINUX_IRQ_WORK_H_

#include <linux/workqueue.h>

/* irq_work runs as ordinary work here */
struct irq_work {
    struct work_struct work;
    void (*func)(struct irq_work *);
};
static inline void __irq_work_run(struct work_struct *w)
{
    struct irq_work *iw = container_of(w, struct irq_work, work);
    iw->func(iw);
}
static inline void init_irq_work(struct irq_work *work, void (*func)(struct irq_work *))
{
    __init_work(&work->work, __irq_work_run);
    work->func = func;
}
#define IRQ_WORK_INIT(f)        { .func = (f) }
static inline bool irq_work_queue(struct irq_work *work)
{
    if (!work->work.func)
        __init_work(&work->work, __irq_work_run);
    return queue_work(system_highpri_wq, &work->work);
}
static inline void irq_work_sync(struct irq_work *work) { flush_work(&work->work); }
#endif
