/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_KTHREAD_H_
#define _LINUX_KTHREAD_H_

#include <linux/sched.h>
#include <linux/err.h>
#include <linux/cgroup.h>

struct task_struct *kthread_run_compat(int (*threadfn)(void *data), void *data, const char *name);
#define kthread_run(threadfn, data, namefmt, ...) kthread_run_compat(threadfn, data, namefmt)
#define kthread_create(threadfn, data, namefmt, ...) kthread_run_compat(threadfn, data, namefmt)
int  kthread_stop(struct task_struct *k);
bool kthread_should_stop(void);
bool kthread_should_park(void);
void kthread_parkme(void);
int  kthread_park(struct task_struct *k);
void kthread_unpark(struct task_struct *k);
#define kthread_bind(t, c)      do { } while (0)
#define wake_up_process_kthread(t) wake_up_process(t)

/* kthread workers: a work queue with the kthread_work item type */
#include <linux/workqueue.h>
struct kthread_work;
typedef void (*kthread_work_func_t)(struct kthread_work *work);
struct kthread_work {
    struct work_struct work;
    kthread_work_func_t func;
};
struct kthread_worker {
    struct workqueue_struct *wq;
};
void __kthread_work_run(struct work_struct *w);
#define kthread_init_work(kw, fn) do { __init_work(&(kw)->work, __kthread_work_run); (kw)->func = (fn); } while (0)
struct kthread_worker *kthread_create_worker(unsigned int flags, const char namefmt[], ...);
#define kthread_run_worker(flags, namefmt, ...) kthread_create_worker(flags, namefmt, ##__VA_ARGS__)
bool kthread_queue_work(struct kthread_worker *worker, struct kthread_work *work);
void kthread_flush_worker(struct kthread_worker *worker);
void kthread_destroy_worker(struct kthread_worker *worker);
bool kthread_cancel_work_sync(struct kthread_work *work);
void kthread_flush_work(struct kthread_work *work);

#endif /* _LINUX_KTHREAD_H_ */
