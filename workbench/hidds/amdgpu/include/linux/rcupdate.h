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

#if defined(__AROSEXEC_SMP__)
/*
 * Forbid() keeps a reader on its CPU, but on SMP it does not stop writers on
 * other CPUs, so readers are also counted and anything retired through
 * call_rcu()/kfree_rcu() is released only once no reader is left anywhere.
 */
extern volatile long compat_rcu_readers;
static inline void rcu_read_lock(void)
{
    Forbid();
    __atomic_add_fetch(&compat_rcu_readers, 1, __ATOMIC_SEQ_CST);
}
static inline void rcu_read_unlock(void)
{
    __atomic_sub_fetch(&compat_rcu_readers, 1, __ATOMIC_SEQ_CST);
    Permit();
}
#else
/*
 * A read-side critical section is a Forbid() window, so a writer can never
 * run while a reader holds a pointer; retired objects are freed at once.
 */
static inline void rcu_read_lock(void)      { Forbid(); }
static inline void rcu_read_unlock(void)    { Permit(); }
#endif
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
void compat_synchronize_rcu(void);
void compat_rcu_barrier(void);
#define synchronize_rcu()               compat_synchronize_rcu()
#define synchronize_rcu_expedited()     synchronize_rcu()
#define rcu_barrier()                   compat_rcu_barrier()
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
typedef void (*rcu_callback_t)(struct rcu_head *head);
void compat_call_rcu(struct rcu_head *head, rcu_callback_t func);
#define call_rcu(head, func)            compat_call_rcu((head), (func))
#define kfree_rcu(ptr, rhf)             compat_call_rcu(&(ptr)->rhf, \
                                            (rcu_callback_t)(unsigned long)__builtin_offsetof(typeof(*(ptr)), rhf))
#define kvfree_rcu(ptr, rhf)            compat_call_rcu(&(ptr)->rhf, \
                                            (rcu_callback_t)((unsigned long)__builtin_offsetof(typeof(*(ptr)), rhf) | 1))
#define kfree_rcu_mightsleep(ptr)       do { synchronize_rcu(); kfree(ptr); } while (0)
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
