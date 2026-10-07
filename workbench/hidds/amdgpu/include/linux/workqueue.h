/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_WORKQUEUE_H_
#define _LINUX_WORKQUEUE_H_

#include <linux/types.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <linux/lockdep.h>
#include <linux/rcupdate.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>

struct workqueue_struct;
struct work_struct;
typedef void (*work_func_t)(struct work_struct *work);

/*
 * Work items run on one worker process; a delayed one first parks on the
 * timer process. state tracks the item through the queue so that flushes
 * and cancels can be answered.
 */
struct work_struct {
    work_func_t func;
    volatile UBYTE state;
    struct workqueue_struct *wq;
    struct MinNode node;
};

struct delayed_work {
    struct work_struct work;
    struct timer_list timer;
};

struct rcu_work {
    struct work_struct work;
};

enum {
    WQ_UNBOUND              = 1 << 1,
    WQ_FREEZABLE            = 1 << 2,
    WQ_MEM_RECLAIM          = 1 << 3,
    WQ_HIGHPRI              = 1 << 4,
    WQ_CPU_INTENSIVE        = 1 << 5,
    WQ_SYSFS                = 1 << 6,
    WQ_POWER_EFFICIENT      = 1 << 7,
    WQ_BH                   = 1 << 8,
    __WQ_ORDERED            = 1 << 17,
    WQ_MAX_ACTIVE           = 512,
    WQ_UNBOUND_MAX_ACTIVE   = WQ_MAX_ACTIVE,
    WQ_DFL_ACTIVE           = WQ_MAX_ACTIVE / 2,
};

extern struct workqueue_struct *system_wq;
extern struct workqueue_struct *system_highpri_wq;
extern struct workqueue_struct *system_long_wq;
extern struct workqueue_struct *system_unbound_wq;
extern struct workqueue_struct *system_freezable_wq;
extern struct workqueue_struct *system_power_efficient_wq;
#define system_dfl_wq           system_unbound_wq
#define system_percpu_wq        system_wq

#define __WORK_INITIALIZER(n, f)        { .func = (f), .state = 0, .wq = NULL }
#define __DELAYED_WORK_INITIALIZER(n, f, tf) { .work = __WORK_INITIALIZER((n).work, (f)) }
#define DECLARE_WORK(n, f)              struct work_struct n = __WORK_INITIALIZER(n, f)
#define DECLARE_DELAYED_WORK(n, f)      struct delayed_work n = __DELAYED_WORK_INITIALIZER(n, f, 0)

void __init_work(struct work_struct *work, work_func_t func);
void __init_delayed_work(struct delayed_work *dwork, work_func_t func);
#define INIT_WORK(_work, _func)         __init_work((_work), (_func))
#define INIT_WORK_ONSTACK(_work, _func) __init_work((_work), (_func))
#define INIT_DELAYED_WORK(_work, _func) __init_delayed_work((_work), (_func))
#define INIT_DELAYED_WORK_ONSTACK(_work, _func) __init_delayed_work((_work), (_func))
#define INIT_DEFERRABLE_WORK(_work, _func) __init_delayed_work((_work), (_func))
#define INIT_RCU_WORK(_work, _func)     __init_work(&(_work)->work, (_func))

static inline struct delayed_work *to_delayed_work(struct work_struct *work)
{
    return container_of(work, struct delayed_work, work);
}
static inline struct rcu_work *to_rcu_work(struct work_struct *work)
{
    return container_of(work, struct rcu_work, work);
}

struct workqueue_struct *alloc_workqueue(const char *fmt, unsigned int flags, int max_active, ...);
#define alloc_ordered_workqueue(fmt, flags, args...) alloc_workqueue(fmt, WQ_UNBOUND | __WQ_ORDERED | (flags), 1, ##args)
#define create_workqueue(name)              alloc_workqueue("%s", WQ_MEM_RECLAIM, 1, (name))
#define create_freezable_workqueue(name)    alloc_workqueue("%s", WQ_FREEZABLE | WQ_UNBOUND | WQ_MEM_RECLAIM, 1, (name))
#define create_singlethread_workqueue(name) alloc_ordered_workqueue("%s", WQ_MEM_RECLAIM, name)
void destroy_workqueue(struct workqueue_struct *wq);

bool queue_work(struct workqueue_struct *wq, struct work_struct *work);
bool queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay);
bool mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay);
bool queue_rcu_work(struct workqueue_struct *wq, struct rcu_work *rwork);
#define queue_work_on(cpu, wq, work)                queue_work(wq, work)
#define queue_work_node(node, wq, work)             queue_work(wq, work)
#define queue_delayed_work_on(cpu, wq, dwork, d)    queue_delayed_work(wq, dwork, d)
#define mod_delayed_work_on(cpu, wq, dwork, d)      mod_delayed_work(wq, dwork, d)
static inline bool schedule_work(struct work_struct *work)                        { return queue_work(system_wq, work); }
static inline bool schedule_delayed_work(struct delayed_work *dwork, unsigned long delay) { return queue_delayed_work(system_wq, dwork, delay); }
#define schedule_work_on(cpu, work)                 schedule_work(work)
#define schedule_delayed_work_on(cpu, dwork, d)     schedule_delayed_work(dwork, d)

void flush_workqueue(struct workqueue_struct *wq);
void drain_workqueue(struct workqueue_struct *wq);
bool flush_work(struct work_struct *work);
bool flush_delayed_work(struct delayed_work *dwork);
bool flush_rcu_work(struct rcu_work *rwork);
bool cancel_work(struct work_struct *work);
bool cancel_work_sync(struct work_struct *work);
bool cancel_delayed_work(struct delayed_work *dwork);
bool cancel_delayed_work_sync(struct delayed_work *dwork);
#define disable_work_sync(w)        cancel_work_sync(w)
#define disable_delayed_work_sync(w) cancel_delayed_work_sync(w)
#define enable_work(w)              do { } while (0)
#define enable_delayed_work(w)      do { } while (0)
#define flush_scheduled_work()      flush_workqueue(system_wq)
bool work_pending(struct work_struct *work);
bool work_busy(struct work_struct *work);
#define delayed_work_pending(w)     work_pending(&(w)->work)
#define current_work()              ((struct work_struct *)NULL)
#define destroy_work_on_stack(w)    do { } while (0)
#define destroy_delayed_work_on_stack(w) do { } while (0)
#define workqueue_set_max_active(wq, n) do { } while (0)
#define work_data_bits(w)           ((unsigned long *)&(w)->state)
#define WORK_STRUCT_PENDING_BIT     0

#endif /* _LINUX_WORKQUEUE_H_ */
