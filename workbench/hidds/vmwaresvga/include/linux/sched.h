/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SCHED_H_
#define _LINUX_SCHED_H_

#include <proto/exec.h>
#include <exec/tasks.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/timer.h>
#include <linux/rbtree.h>
#include <linux/refcount.h>
#include <linux/hrtimer.h>

/*
 * A Linux task is an exec Task; the sleep/wake protocol maps onto
 * SIGF_SINGLE. set_current_state() clears the signal, schedule() waits
 * for it, wake_up_process() sends it - so a wakeup that lands between the
 * state change and the sleep is not lost, exactly as on Linux.
 */
#define TASK_COMM_LEN           16
/*
 * The per-task record behind current: the exec task plus the few fields
 * Linux code reads directly.
 */
struct task_struct {
    struct Task *task;
    char comm[TASK_COMM_LEN];
    int pid;
    struct list_head node;
};
struct task_struct *compat_current(void);
#define current                 (compat_current())
#define task_pid_nr(t)          ((t)->pid)
#define task_tgid_nr(t)         ((t)->pid)
#define task_pid_vnr(t)         ((t)->pid)
#define get_task_comm(buf, t)   strscpy(buf, (t)->comm, TASK_COMM_LEN)
#define task_pid(t)             ((struct pid *)(t))
#define get_task_pid(t, type)   ((struct pid *)(t))
#define pid_nr(p)               ((int)(IPTR)(p) ? ((struct task_struct *)(p))->pid : 0)
#define put_pid(p)              do { } while (0)
#define pid_vnr(p)              pid_nr(p)
#define PIDTYPE_TGID            0
#define PIDTYPE_PID             0
struct pid;
#define get_task_struct(t)      (t)
#define put_task_struct(t)      do { } while (0)

#define TASK_RUNNING            0x0000
#define TASK_INTERRUPTIBLE      0x0001
#define TASK_UNINTERRUPTIBLE    0x0002
#define TASK_KILLABLE           0x0102
#define TASK_IDLE               0x0402
#define TASK_NORMAL             (TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)
#define TASK_WAKEKILL           0x0100
#define TASK_NOLOAD             0x0400
#define TASK_FREEZABLE          0x0800

void __set_current_state(long state);
#define set_current_state(s)    __set_current_state(s)
void schedule(void);
signed long schedule_timeout(signed long timeout);
signed long schedule_timeout_interruptible(signed long timeout);
signed long schedule_timeout_uninterruptible(signed long timeout);
signed long schedule_timeout_killable(signed long timeout);
int schedule_hrtimeout(ktime_t *expires, const enum hrtimer_mode mode);
int schedule_hrtimeout_range(ktime_t *expires, u64 delta, const enum hrtimer_mode mode);
int  wake_up_process(struct task_struct *p);
#define wake_up_state(p, s)     wake_up_process(p)
static inline int signal_pending(struct task_struct *p)     { (void)p; return 0; }
static inline int fatal_signal_pending(struct task_struct *p) { (void)p; return 0; }
static inline int signal_pending_state(long s, struct task_struct *p) { (void)s; (void)p; return 0; }
#define need_resched()          (0)
#define yield()                 do { } while (0)
#define io_schedule()           schedule()
#define io_schedule_timeout(t)  schedule_timeout(t)
#define set_user_nice(t, n)     do { } while (0)
#define sched_setscheduler(...) (0)
#define sched_set_fifo(t)       do { } while (0)
#define sched_set_normal(t, n)  do { } while (0)
#define cond_resched_lock(l)    (0)
#define PF_KTHREAD              0
#define PF_MEMALLOC             0
#define freezing(t)             (0)
#define try_to_freeze()         (0)
#define set_freezable()         do { } while (0)
#define kthread_freezable_should_stop(w) kthread_should_stop()

#endif /* _LINUX_SCHED_H_ */
