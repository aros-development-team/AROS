/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/rcupdate.h>
#include <aros/debug.h>

extern unsigned long compat_sleep_usecs(unsigned long usecs);

#if defined(__AROSEXEC_SMP__)
volatile long compat_rcu_readers;
#endif

static DEFINE_RAW_SPINLOCK(rcu_pending_lock);
static struct rcu_head *rcu_pending_head;
static struct rcu_head **rcu_pending_tail = &rcu_pending_head;
static struct work_struct rcu_reaper;
static struct workqueue_struct *rcu_wq;

static inline long rcu_readers(void)
{
#if defined(__AROSEXEC_SMP__)
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    return __atomic_load_n(&compat_rcu_readers, __ATOMIC_SEQ_CST);
#else
    return 0;
#endif
}

static void rcu_wait_for_readers(void)
{
    unsigned long slept = 0;
    int spins = 0;
    static bool reported;

    while (rcu_readers() != 0) {
        if (++spins < 2000) {
            cpu_relax();
            continue;
        }
        compat_sleep_usecs(20);
        slept += 20;
        if (slept >= 5000000 && !reported) {
            reported = true;
            bug("[amdgpu] rcu: %ld reader(s) still inside rcu_read_lock after 5 s\n", rcu_readers());
        }
    }
}

static void rcu_invoke(struct rcu_head *head)
{
    unsigned long off = (unsigned long)head->func;

    if (off < 4096) {
        if (off & 1)
            kvfree((char *)head - (off & ~1UL));
        else
            kfree((char *)head - off);
    } else
        head->func(head);
}

static void rcu_reaper_fn(struct work_struct *work)
{
    for (;;) {
        unsigned long flags;
        struct rcu_head *batch;

        raw_spin_lock_irqsave(&rcu_pending_lock, flags);
        batch = rcu_pending_head;
        rcu_pending_head = NULL;
        rcu_pending_tail = &rcu_pending_head;
        raw_spin_unlock_irqrestore(&rcu_pending_lock, flags);
        if (!batch)
            break;

        rcu_wait_for_readers();

        while (batch) {
            struct rcu_head *next = batch->next;
            rcu_invoke(batch);
            batch = next;
        }
    }
}

void compat_call_rcu(struct rcu_head *head, rcu_callback_t func)
{
    unsigned long flags;

    head->func = func;
    if (rcu_readers() == 0) {
        rcu_invoke(head);
        return;
    }

    head->next = NULL;
    raw_spin_lock_irqsave(&rcu_pending_lock, flags);
    *rcu_pending_tail = head;
    rcu_pending_tail = (struct rcu_head **)&head->next;
    raw_spin_unlock_irqrestore(&rcu_pending_lock, flags);

    if (rcu_wq)
        queue_work(rcu_wq, &rcu_reaper);
}

void compat_rcu_init(void)
{
#if defined(__AROSEXEC_SMP__)
    if (rcu_wq)
        return;
    INIT_WORK(&rcu_reaper, rcu_reaper_fn);
    rcu_wq = alloc_workqueue("Amdgpu RCU", 0, 0);
    if (rcu_wq && rcu_pending_head)
        queue_work(rcu_wq, &rcu_reaper);
#endif
}

void compat_synchronize_rcu(void)
{
    rcu_wait_for_readers();
}

void compat_rcu_barrier(void)
{
    rcu_wait_for_readers();
    if (rcu_wq) {
        queue_work(rcu_wq, &rcu_reaper);
        flush_work(&rcu_reaper);
    } else
        rcu_reaper_fn(NULL);
}
