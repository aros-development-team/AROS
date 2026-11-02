/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Deferred frees for the RCU emulation (see include/linux/rcupdate.h).
    A fixed ring of pending objects; a timer moves the drain onto the system
    workqueue task once the oldest entry has aged RCU_GRACE_MS, so no memory
    is freed from interrupt context and no reader still inside its Forbid()
    window can be holding the pointer. Readers never sleep in their critical
    sections (they run Forbid()den), so a quarter of a second is a very wide
    grace period. A full ring frees the oldest entry at once - a degradation,
    reported once.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <linux/kernel.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/spinlock_types.h>

#define RCU_GRACE_MS    250
#define RCU_RING        4096

struct rcu_pending {
    void *ptr;                              /* kfree this, or */
    struct rcu_head *head;                  /* call func(head) */
    void (*func)(struct rcu_head *);
    unsigned long when;
};

static struct rcu_pending rcu_ring[RCU_RING];
static unsigned int rcu_head_i, rcu_tail_i;     /* tail: oldest, head: next free */
static struct nouveau_krnspin rcu_lock;
static struct timer_list rcu_timer;
static struct work_struct rcu_work;
static int rcu_ready, rcu_full_reported;

static inline void rcu_lock_acquire(void)
{
    Disable();
    if (KernelBase)
        KrnSpinLock((void *)&rcu_lock, NULL, SPINLOCK_MODE_WRITE);
}

static inline void rcu_lock_release(void)
{
    if (KernelBase)
        KrnSpinUnLock((void *)&rcu_lock);
    Enable();
}

static void rcu_drain_work(struct work_struct *w);

static void rcu_timer_fn(struct timer_list *t)
{
    schedule_work(&rcu_work);
}

static void rcu_init_locked(void)
{
    if (rcu_ready)
        return;
    rcu_lock.lock = 0;
    rcu_lock.s_Owner = NULL;
    timer_setup(&rcu_timer, rcu_timer_fn, 0);
    INIT_WORK(&rcu_work, rcu_drain_work);
    rcu_ready = 1;
}

static void rcu_run(struct rcu_pending *p)
{
    if (p->func)
        p->func(p->head);
    else
        kfree(p->ptr);
}

/* Free everything older than the grace period; re-arm if anything is left */
static void rcu_drain_work(struct work_struct *w)
{
    unsigned long now = jiffies;
    struct rcu_pending p;
    int more;

    for (;;) {
        rcu_lock_acquire();
        if (rcu_tail_i == rcu_head_i) {
            rcu_lock_release();
            return;
        }
        p = rcu_ring[rcu_tail_i];
        if (!time_after(now, p.when + (RCU_GRACE_MS * HZ) / 1000)) {
            rcu_lock_release();
            break;
        }
        rcu_tail_i = (rcu_tail_i + 1) % RCU_RING;
        rcu_lock_release();
        rcu_run(&p);
    }
    rcu_lock_acquire();
    more = (rcu_tail_i != rcu_head_i);
    rcu_lock_release();
    if (more)
        mod_timer(&rcu_timer, jiffies + (RCU_GRACE_MS * HZ) / 2000);
}

static void rcu_defer(void *ptr, struct rcu_head *head, void (*func)(struct rcu_head *))
{
    struct rcu_pending old = { 0 };
    int evicted = 0, first;

    rcu_lock_acquire();
    rcu_init_locked();
    if ((rcu_head_i + 1) % RCU_RING == rcu_tail_i) {
        old = rcu_ring[rcu_tail_i];
        rcu_tail_i = (rcu_tail_i + 1) % RCU_RING;
        evicted = 1;
    }
    first = (rcu_head_i == rcu_tail_i);
    rcu_ring[rcu_head_i].ptr  = ptr;
    rcu_ring[rcu_head_i].head = head;
    rcu_ring[rcu_head_i].func = func;
    rcu_ring[rcu_head_i].when = jiffies;
    rcu_head_i = (rcu_head_i + 1) % RCU_RING;
    rcu_lock_release();

    if (evicted) {
        if (!rcu_full_reported++)
            bug("[nouveau] rcu: %d deferred frees pending - freeing the oldest early\n", RCU_RING);
        rcu_run(&old);
    }
    if (first)
        mod_timer(&rcu_timer, jiffies + (RCU_GRACE_MS * HZ) / 1000 + 1);
}

void nouveau_rcu_free(void *ptr)
{
    if (ptr)
        rcu_defer(ptr, NULL, NULL);
}

void nouveau_call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *))
{
    rcu_defer(NULL, head, func);
}

void nouveau_rcu_synchronize(void)
{
    /* every reader that was inside its Forbid() window has long since left */
    msleep(RCU_GRACE_MS);
}
