/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_WAIT_H_
#define _LINUX_WAIT_H_

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/stddef.h>
#include <linux/current.h>

struct wait_queue_entry;
typedef int (*wait_queue_func_t)(struct wait_queue_entry *wq_entry, unsigned mode, int flags, void *key);

struct wait_queue_entry {
    unsigned int flags;
    void *private;
    wait_queue_func_t func;
    struct list_head entry;
};
typedef struct wait_queue_entry wait_queue_entry_t;

struct wait_queue_head {
    spinlock_t lock;
    struct list_head head;
};
typedef struct wait_queue_head wait_queue_head_t;

#define WQ_FLAG_EXCLUSIVE       0x01
#define WQ_FLAG_WOKEN           0x02

int default_wake_function(struct wait_queue_entry *wq_entry, unsigned mode, int flags, void *key);
int autoremove_wake_function(struct wait_queue_entry *wq_entry, unsigned mode, int sync, void *key);
int woken_wake_function(struct wait_queue_entry *wq_entry, unsigned mode, int sync, void *key);

#define __WAITQUEUE_INITIALIZER(name, tsk) { .private = tsk, .func = default_wake_function, .entry = { NULL, NULL } }
#define DECLARE_WAITQUEUE(name, tsk)  struct wait_queue_entry name = __WAITQUEUE_INITIALIZER(name, tsk)
#define __WAIT_QUEUE_HEAD_INITIALIZER(name) { .lock = __SPIN_LOCK_UNLOCKED(name.lock), .head = { &(name).head, &(name).head } }
#define DECLARE_WAIT_QUEUE_HEAD(name) struct wait_queue_head name = __WAIT_QUEUE_HEAD_INITIALIZER(name)
#define DECLARE_WAIT_QUEUE_HEAD_ONSTACK(name) DECLARE_WAIT_QUEUE_HEAD(name)
#define DEFINE_WAIT_FUNC(name, function) struct wait_queue_entry name = { .private = current, .func = function, .entry = { &(name).entry, &(name).entry } }
#define DEFINE_WAIT(name)       DEFINE_WAIT_FUNC(name, autoremove_wake_function)

static inline void init_waitqueue_head(struct wait_queue_head *wq_head)
{
    spin_lock_init(&wq_head->lock);
    INIT_LIST_HEAD(&wq_head->head);
}
#define __init_waitqueue_head(wq, name, key) init_waitqueue_head(wq)
static inline void init_waitqueue_entry(struct wait_queue_entry *wq_entry, struct task_struct *p)
{
    wq_entry->flags = 0;
    wq_entry->private = p;
    wq_entry->func = default_wake_function;
}
static inline void init_waitqueue_func_entry(struct wait_queue_entry *wq_entry, wait_queue_func_t func)
{
    wq_entry->flags = 0;
    wq_entry->private = NULL;
    wq_entry->func = func;
}
static inline int waitqueue_active(struct wait_queue_head *wq_head)
{
    return !list_empty(&wq_head->head);
}
static inline bool wq_has_sleeper(struct wait_queue_head *wq_head)
{
    return waitqueue_active(wq_head);
}

void add_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry);
void add_wait_queue_exclusive(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry);
void remove_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry);
void __wake_up(struct wait_queue_head *wq_head, unsigned int mode, int nr, void *key);
void __wake_up_locked(struct wait_queue_head *wq_head, unsigned int mode, int nr);
void prepare_to_wait(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry, int state);
bool prepare_to_wait_exclusive(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry, int state);
long prepare_to_wait_event(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry, int state);
void finish_wait(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry);
long wait_woken(struct wait_queue_entry *wq_entry, unsigned mode, long timeout);

#define wake_up(x)                      __wake_up(x, TASK_NORMAL, 1, NULL)
#define wake_up_nr(x, nr)               __wake_up(x, TASK_NORMAL, nr, NULL)
#define wake_up_all(x)                  __wake_up(x, TASK_NORMAL, 0, NULL)
#define wake_up_locked(x)               __wake_up_locked(x, TASK_NORMAL, 1)
#define wake_up_all_locked(x)           __wake_up_locked(x, TASK_NORMAL, 0)
#define wake_up_interruptible(x)        __wake_up(x, TASK_INTERRUPTIBLE, 1, NULL)
#define wake_up_interruptible_nr(x, nr) __wake_up(x, TASK_INTERRUPTIBLE, nr, NULL)
#define wake_up_interruptible_all(x)    __wake_up(x, TASK_INTERRUPTIBLE, 0, NULL)
#define wake_up_interruptible_sync(x)   wake_up_interruptible(x)
#define wake_up_interruptible_poll(x, m) __wake_up(x, TASK_INTERRUPTIBLE, 1, (void *)(IPTR)(m))
#define wake_up_poll(x, m)              __wake_up(x, TASK_NORMAL, 1, (void *)(IPTR)(m))

/*
 * The event macros follow the Linux shape so that a wakeup racing with the
 * condition check is caught by the latched signal.
 */
#define ___wait_cond_timeout(condition)                                 \
    ({ bool __cond = (condition); if (__cond && !__ret) __ret = 1; __cond || !__ret; })

#define ___wait_event(wq_head, condition, state, exclusive, ret, cmd)   \
    ({                                                                  \
        __label__ __out;                                                \
        struct wait_queue_entry __wq_entry;                             \
        long __ret = ret;                                               \
        init_waitqueue_entry(&__wq_entry, current);                     \
        if (exclusive) __wq_entry.flags = WQ_FLAG_EXCLUSIVE;            \
        for (;;) {                                                      \
            long __int = prepare_to_wait_event(&(wq_head), &__wq_entry, state); \
            if (condition)                                              \
                break;                                                  \
            if (__int) { __ret = __int; goto __out; }                   \
            cmd;                                                        \
        }                                                               \
        finish_wait(&(wq_head), &__wq_entry);                           \
    __out:  __ret;                                                      \
    })

#define __wait_event(wq_head, condition)                                \
    (void)___wait_event(wq_head, condition, TASK_UNINTERRUPTIBLE, 0, 0, schedule())
#define wait_event(wq_head, condition)                                  \
    do { if (condition) break; __wait_event(wq_head, condition); } while (0)

#define __wait_event_timeout(wq_head, condition, timeout)               \
    ___wait_event(wq_head, ___wait_cond_timeout(condition), TASK_UNINTERRUPTIBLE, 0, timeout, __ret = schedule_timeout(__ret))
#define wait_event_timeout(wq_head, condition, timeout)                 \
    ({ long __ret = timeout; if (!___wait_cond_timeout(condition)) __ret = __wait_event_timeout(wq_head, condition, timeout); __ret; })

#define __wait_event_interruptible(wq_head, condition)                  \
    ___wait_event(wq_head, condition, TASK_INTERRUPTIBLE, 0, 0, schedule())
#define wait_event_interruptible(wq_head, condition)                    \
    ({ int __ret = 0; if (!(condition)) __ret = __wait_event_interruptible(wq_head, condition); __ret; })

#define __wait_event_interruptible_timeout(wq_head, condition, timeout) \
    ___wait_event(wq_head, ___wait_cond_timeout(condition), TASK_INTERRUPTIBLE, 0, timeout, __ret = schedule_timeout(__ret))
#define wait_event_interruptible_timeout(wq_head, condition, timeout)   \
    ({ long __ret = timeout; if (!___wait_cond_timeout(condition)) __ret = __wait_event_interruptible_timeout(wq_head, condition, timeout); __ret; })

#define wait_event_killable(wq_head, condition)             wait_event_interruptible(wq_head, condition)
#define wait_event_killable_timeout(wq_head, condition, t)  wait_event_interruptible_timeout(wq_head, condition, t)
#define wait_event_freezable(wq_head, condition)            wait_event_interruptible(wq_head, condition)
#define wait_event_freezable_timeout(wq_head, condition, t) wait_event_interruptible_timeout(wq_head, condition, t)
#define wait_event_idle(wq_head, condition)                 wait_event(wq_head, condition)
#define wait_event_idle_timeout(wq_head, condition, t)      wait_event_timeout(wq_head, condition, t)
#define wait_event_interruptible_locked(wq, condition)      wait_event_interruptible(wq, condition)
#define wait_event_interruptible_locked_irq(wq, condition)  wait_event_interruptible(wq, condition)

#define __wait_event_lock_irq(wq_head, condition, lock, cmd)            \
    (void)___wait_event(wq_head, condition, TASK_UNINTERRUPTIBLE, 0, 0, \
        spin_unlock_irq(&lock); cmd; schedule(); spin_lock_irq(&lock))
#define wait_event_lock_irq(wq_head, condition, lock)                   \
    do { if (condition) break; __wait_event_lock_irq(wq_head, condition, lock, ); } while (0)
#define wait_event_lock_irq_cmd(wq_head, condition, lock, cmd)          \
    do { if (condition) break; __wait_event_lock_irq(wq_head, condition, lock, cmd); } while (0)
#define wait_event_interruptible_lock_irq(wq_head, condition, lock)     \
    ({ int __ret = 0; if (!(condition)) __ret = ___wait_event(wq_head, condition, TASK_INTERRUPTIBLE, 0, 0, \
        spin_unlock_irq(&lock); schedule(); spin_lock_irq(&lock)); __ret; })

/* wait_bit: sleeping on a bit in a word, keyed by address */
struct wait_bit_key {
    void *flags;
    int bit_nr;
    unsigned long timeout;
};
struct wait_bit_queue_entry {
    struct wait_bit_key key;
    struct wait_queue_entry wq_entry;
};
#define wake_up_bit(w, b)               do { } while (0)
#define wake_up_var(v)                  do { } while (0)
#define wait_on_bit(w, b, m)            (0)
#define wait_on_bit_timeout(w, b, m, t) (0)
#define wait_var_event(v, c)            do { } while (0)
#define wait_var_event_timeout(v, c, t) (t)
#define clear_and_wake_up_bit(b, w)     clear_bit(b, w)

#include <linux/completion.h>

#endif /* _LINUX_WAIT_H_ */
