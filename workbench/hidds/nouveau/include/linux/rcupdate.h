/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_RCUPDATE_H_
#define _LINUX_RCUPDATE_H_

#include <proto/exec.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/irqflags.h>
#include <linux/preempt.h>
#include <linux/bottom_half.h>
#include <linux/lockdep.h>
#include <linux/cleanup.h>
#include <linux/rcupdate_trace.h>

/*
 * A read-side critical section is a Forbid() window. That keeps the reader
 * from being rescheduled, but on SMP a writer on another CPU runs anyway:
 * freeing "after a grace period" has to be a real deferral, or an unlocked
 * walker (dma_resv_iter_walk_unlocked) reads a freed fence table, finds
 * fences with no references and restarts for ever inside Forbid() - the
 * Titan 'ttm' task spinning Forbid()den, 2026-11-01. linux_rcu.c holds the
 * objects for RCU_GRACE_MS and frees them from the system workqueue.
 */
void nouveau_rcu_free(void *ptr);
void nouveau_call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *));
void nouveau_rcu_synchronize(void);
static inline void rcu_read_lock(void)      { Forbid(); }
static inline void rcu_read_unlock(void)    { Permit(); }
#define rcu_read_lock_bh()              rcu_read_lock()
#define rcu_read_unlock_bh()            rcu_read_unlock()
#define rcu_read_lock_sched()           rcu_read_lock()
#define rcu_read_unlock_sched()         rcu_read_unlock()
#define rcu_read_lock_held()            (1)
#define rcu_read_lock_bh_held()         (1)
#define rcu_read_lock_sched_held()      (1)
#define rcu_read_lock_any_held()        (1)
#define rcu_lockdep_assert(c, s)        do { } while (0)
#define RCU_LOCKDEP_WARN(c, s)          do { } while (0)
#define rcu_sleep_check()               do { } while (0)
#define synchronize_rcu()               nouveau_rcu_synchronize()
#define synchronize_rcu_expedited()     synchronize_rcu()
#define rcu_barrier()                   synchronize_rcu()
#define rcu_dereference(p)              (p)
#define rcu_dereference_raw(p)          (p)
#define rcu_dereference_bh(p)           (p)
#define rcu_dereference_sched(p)        (p)
#define rcu_dereference_check(p, c)     (p)
#define rcu_dereference_protected(p, c) (p)
#define rcu_access_pointer(p)           (p)
#define rcu_pointer_handoff(p)          (p)
#define rcu_assign_pointer(p, v)        do { __sync_synchronize(); (p) = (v); } while (0)
#define RCU_INIT_POINTER(p, v)          do { (p) = (v); } while (0)
#define rcu_replace_pointer(rp, p, c)   ({ typeof(rp) __r = (rp); rcu_assign_pointer(rp, p); __r; })
#define rcu_swap_protected(rp, p, c)    do { typeof(p) __t = (rp); rcu_assign_pointer(rp, p); (p) = __t; } while (0)
#define unrcu_pointer(p)                (p)
#define kfree_rcu(ptr, ...)             nouveau_rcu_free(ptr)
#define kvfree_rcu(ptr, ...)            nouveau_rcu_free(ptr)
#define kfree_rcu_mightsleep(ptr)       nouveau_rcu_free(ptr)
typedef void (*rcu_callback_t)(struct rcu_head *head);
static inline void call_rcu(struct rcu_head *head, rcu_callback_t func) { nouveau_call_rcu(head, func); }
#define init_rcu_head(h)                do { } while (0)
#define destroy_rcu_head(h)             do { } while (0)
#define init_rcu_head_on_stack(h)       do { } while (0)
#define destroy_rcu_head_on_stack(h)    do { } while (0)
#define rcu_head_init(h)                do { } while (0)
#define cond_synchronize_rcu(x)         do { } while (0)
#define get_state_synchronize_rcu()     (0UL)
#define __rcu_dereference_check(p, c)   (p)

#include <linux/cleanup.h>
DEFINE_LOCK_GUARD_0(rcu, rcu_read_lock(), rcu_read_unlock())

#endif /* _LINUX_RCUPDATE_H_ */
