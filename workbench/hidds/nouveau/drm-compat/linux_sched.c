/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <exec/ports.h>
#include <devices/timer.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <stdarg.h>

/*
 * The Linux sleep protocol - mark the task, check the condition, sleep -
 * is carried by SIGF_SINGLE: marking clears the signal, sleeping waits
 * for it, waking sends it. A wakeup that arrives between the check and
 * the sleep is latched in the signal mask, so it is not lost.
 */

extern struct Device *TimerBase;
extern struct timerequest *compat_timer_template(void);

void __set_current_state(long state)
{
    if (state != TASK_RUNNING)
        SetSignal(0, SIGF_SINGLE);
}

/*
 * One record per exec task, made on first use and kept for the life of the
 * driver; the list is short.
 */
static LIST_HEAD(task_records);
static int next_pid = 1;

struct task_struct *compat_current(void)
{
    struct Task *task = FindTask(NULL);
    struct task_struct *ts;

    Forbid();
    list_for_each_entry(ts, &task_records, node) {
        if (ts->task == task) {
            Permit();
            return ts;
        }
    }
    Permit();

    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (!ts)
        return NULL;
    ts->task = task;
    ts->pid = next_pid++;
    if (task->tc_Node.ln_Name)
        strscpy(ts->comm, task->tc_Node.ln_Name, sizeof(ts->comm));
    Forbid();
    list_add(&ts->node, &task_records);
    Permit();
    return ts;
}

int wake_up_process(struct task_struct *p)
{
    Signal(p->task, SIGF_SINGLE);
    return 1;
}

void schedule(void)
{
    Wait(SIGF_SINGLE);
}

signed long schedule_timeout(signed long timeout)
{
    struct timerequest req;
    struct MsgPort port;
    unsigned long usecs;
    ULONG sigs;
    unsigned long start;

    if (timeout == MAX_SCHEDULE_TIMEOUT) {
        Wait(SIGF_SINGLE);
        return timeout;
    }
    if (timeout <= 0)
        return 0;
    if (!TimerBase) {
        Wait(SIGF_SINGLE);
        return timeout;
    }

    start = jiffies;
    usecs = jiffies_to_usecs(timeout);

    memset(&port, 0, sizeof(port));
    port.mp_Node.ln_Type = NT_MSGPORT;
    port.mp_Flags = PA_SIGNAL;
    port.mp_SigBit = AllocSignal(-1);
    port.mp_SigTask = FindTask(NULL);
    NEWLIST(&port.mp_MsgList);
    if (port.mp_SigBit == (UBYTE)-1) {
        Wait(SIGF_SINGLE);
        return timeout;
    }

    req = *compat_timer_template();
    req.tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    req.tr_node.io_Message.mn_ReplyPort = &port;
    req.tr_node.io_Command = TR_ADDREQUEST;
    req.tr_time.tv_secs = usecs / 1000000;
    req.tr_time.tv_micro = usecs % 1000000;
    SendIO((struct IORequest *)&req);

    sigs = Wait(SIGF_SINGLE | (1UL << port.mp_SigBit));
    if (!CheckIO((struct IORequest *)&req))
        AbortIO((struct IORequest *)&req);
    WaitIO((struct IORequest *)&req);
    FreeSignal(port.mp_SigBit);

    if (sigs & SIGF_SINGLE) {
        unsigned long spent = jiffies - start;
        return spent < (unsigned long)timeout ? timeout - spent : 1;
    }
    return 0;
}

signed long schedule_timeout_interruptible(signed long timeout)
{
    __set_current_state(TASK_INTERRUPTIBLE);
    return schedule_timeout(timeout);
}

signed long schedule_timeout_uninterruptible(signed long timeout)
{
    __set_current_state(TASK_UNINTERRUPTIBLE);
    return schedule_timeout(timeout);
}

signed long schedule_timeout_killable(signed long timeout)
{
    __set_current_state(TASK_KILLABLE);
    return schedule_timeout(timeout);
}

int schedule_hrtimeout_range(ktime_t *expires, u64 delta, const enum hrtimer_mode mode)
{
    ktime_t now = ktime_get();
    ktime_t exp = *expires;
    s64 delta_ns;

    if (!(mode & HRTIMER_MODE_ABS))
        exp += now;
    delta_ns = exp - now;
    if (delta_ns <= 0)
        return 0;
    schedule_timeout(nsecs_to_jiffies(delta_ns) ?: 1);
    return 0;
}

int schedule_hrtimeout(ktime_t *expires, const enum hrtimer_mode mode)
{
    return schedule_hrtimeout_range(expires, 0, mode);
}

/* --- wait queues ---------------------------------------------------- */

int default_wake_function(struct wait_queue_entry *wq_entry, unsigned mode, int flags, void *key)
{
    return wake_up_process(wq_entry->private);
}

int autoremove_wake_function(struct wait_queue_entry *wq_entry, unsigned mode, int sync, void *key)
{
    int ret = default_wake_function(wq_entry, mode, sync, key);
    if (ret)
        list_del_init(&wq_entry->entry);
    return ret;
}

int woken_wake_function(struct wait_queue_entry *wq_entry, unsigned mode, int sync, void *key)
{
    wq_entry->flags |= WQ_FLAG_WOKEN;
    return default_wake_function(wq_entry, mode, sync, key);
}

void add_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry)
{
    unsigned long flags;

    wq_entry->flags &= ~WQ_FLAG_EXCLUSIVE;
    spin_lock_irqsave(&wq_head->lock, flags);
    list_add(&wq_entry->entry, &wq_head->head);
    spin_unlock_irqrestore(&wq_head->lock, flags);
}

void add_wait_queue_exclusive(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry)
{
    unsigned long flags;

    wq_entry->flags |= WQ_FLAG_EXCLUSIVE;
    spin_lock_irqsave(&wq_head->lock, flags);
    list_add_tail(&wq_entry->entry, &wq_head->head);
    spin_unlock_irqrestore(&wq_head->lock, flags);
}

void remove_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry)
{
    unsigned long flags;

    spin_lock_irqsave(&wq_head->lock, flags);
    list_del(&wq_entry->entry);
    spin_unlock_irqrestore(&wq_head->lock, flags);
}

void __wake_up_locked(struct wait_queue_head *wq_head, unsigned int mode, int nr)
{
    struct wait_queue_entry *curr, *next;

    list_for_each_entry_safe(curr, next, &wq_head->head, entry) {
        unsigned int flags = curr->flags;
        int ret = curr->func(curr, mode, 0, NULL);
        if (ret < 0)
            break;
        if (ret && (flags & WQ_FLAG_EXCLUSIVE) && !--nr)
            break;
    }
}

void __wake_up(struct wait_queue_head *wq_head, unsigned int mode, int nr, void *key)
{
    unsigned long flags;

    spin_lock_irqsave(&wq_head->lock, flags);
    __wake_up_locked(wq_head, mode, nr);
    spin_unlock_irqrestore(&wq_head->lock, flags);
}

void prepare_to_wait(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry, int state)
{
    unsigned long flags;

    wq_entry->flags &= ~WQ_FLAG_EXCLUSIVE;
    spin_lock_irqsave(&wq_head->lock, flags);
    if (list_empty(&wq_entry->entry))
        list_add(&wq_entry->entry, &wq_head->head);
    __set_current_state(state);
    spin_unlock_irqrestore(&wq_head->lock, flags);
}

bool prepare_to_wait_exclusive(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry, int state)
{
    unsigned long flags;
    bool was_empty = false;

    wq_entry->flags |= WQ_FLAG_EXCLUSIVE;
    spin_lock_irqsave(&wq_head->lock, flags);
    if (list_empty(&wq_entry->entry)) {
        was_empty = list_empty(&wq_head->head);
        list_add_tail(&wq_entry->entry, &wq_head->head);
    }
    __set_current_state(state);
    spin_unlock_irqrestore(&wq_head->lock, flags);
    return was_empty;
}

long prepare_to_wait_event(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry, int state)
{
    unsigned long flags;

    spin_lock_irqsave(&wq_head->lock, flags);
    if (list_empty(&wq_entry->entry) || wq_entry->entry.next == NULL) {
        if (wq_entry->flags & WQ_FLAG_EXCLUSIVE)
            list_add_tail(&wq_entry->entry, &wq_head->head);
        else
            list_add(&wq_entry->entry, &wq_head->head);
    }
    __set_current_state(state);
    spin_unlock_irqrestore(&wq_head->lock, flags);
    return 0;
}

void finish_wait(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry)
{
    unsigned long flags;

    spin_lock_irqsave(&wq_head->lock, flags);
    if (!list_empty(&wq_entry->entry))
        list_del_init(&wq_entry->entry);
    spin_unlock_irqrestore(&wq_head->lock, flags);
}

long wait_woken(struct wait_queue_entry *wq_entry, unsigned mode, long timeout)
{
    __set_current_state(mode);
    if (!(wq_entry->flags & WQ_FLAG_WOKEN))
        timeout = schedule_timeout(timeout);
    wq_entry->flags &= ~WQ_FLAG_WOKEN;
    return timeout;
}

/* --- completions ------------------------------------------------------ */

void complete(struct completion *x)
{
    unsigned long flags;

    spin_lock_irqsave(&x->wait.lock, flags);
    if (x->done != UINT_MAX)
        x->done++;
    __wake_up_locked(&x->wait, TASK_NORMAL, 1);
    spin_unlock_irqrestore(&x->wait.lock, flags);
}

void complete_all(struct completion *x)
{
    unsigned long flags;

    spin_lock_irqsave(&x->wait.lock, flags);
    x->done = UINT_MAX;
    __wake_up_locked(&x->wait, TASK_NORMAL, 0);
    spin_unlock_irqrestore(&x->wait.lock, flags);
}

static long do_wait_for_common(struct completion *x, long timeout, int state)
{
    if (!x->done) {
        DECLARE_WAITQUEUE(wait, current);

        add_wait_queue_exclusive(&x->wait, &wait);
        do {
            __set_current_state(state);
            spin_unlock_irq(&x->wait.lock);
            timeout = schedule_timeout(timeout);
            spin_lock_irq(&x->wait.lock);
        } while (!x->done && timeout);
        remove_wait_queue(&x->wait, &wait);
        if (!x->done)
            return timeout;
    }
    if (x->done != UINT_MAX)
        x->done--;
    return timeout ?: 1;
}

static long wait_for_common(struct completion *x, long timeout, int state)
{
    spin_lock_irq(&x->wait.lock);
    timeout = do_wait_for_common(x, timeout, state);
    spin_unlock_irq(&x->wait.lock);
    return timeout;
}

void wait_for_completion(struct completion *x)
{
    wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_UNINTERRUPTIBLE);
}

unsigned long wait_for_completion_timeout(struct completion *x, unsigned long timeout)
{
    return wait_for_common(x, timeout, TASK_UNINTERRUPTIBLE);
}

int wait_for_completion_interruptible(struct completion *x)
{
    wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_INTERRUPTIBLE);
    return 0;
}

long wait_for_completion_interruptible_timeout(struct completion *x, unsigned long timeout)
{
    return wait_for_common(x, timeout, TASK_INTERRUPTIBLE);
}

bool try_wait_for_completion(struct completion *x)
{
    unsigned long flags;
    bool ret = true;

    if (!READ_ONCE(x->done))
        return false;

    spin_lock_irqsave(&x->wait.lock, flags);
    if (!x->done)
        ret = false;
    else if (x->done != UINT_MAX)
        x->done--;
    spin_unlock_irqrestore(&x->wait.lock, flags);
    return ret;
}

bool completion_done(struct completion *x)
{
    return READ_ONCE(x->done) != 0;
}

/* --- kthreads ---------------------------------------------------------- */

struct kthread_start {
    int (*fn)(void *);
    void *data;
    struct Task *parent;
    struct Task *self;
    struct task_struct *ts;
    volatile BOOL should_stop;
    int result;
};

static void kthread_entry(void)
{
    struct Task *self = FindTask(NULL);
    struct kthread_start *ks = self->tc_UserData;

    ks->self = self;
    ks->ts = compat_current();
    Signal(ks->parent, SIGF_SINGLE);
    ks->result = ks->fn(ks->data);
}

struct task_struct *kthread_run_compat(int (*threadfn)(void *data), void *data, const char *name)
{
    struct kthread_start *ks = kzalloc(sizeof(*ks), GFP_KERNEL);
    struct Process *proc;

    if (!ks)
        return ERR_PTR(-ENOMEM);
    ks->fn = threadfn;
    ks->data = data;
    ks->parent = FindTask(NULL);

    proc = CreateNewProcTags(
        NP_Name, (IPTR)name,
        NP_Priority, 5,
        NP_Entry, (IPTR)kthread_entry,
        NP_StackSize, 128 * 1024,
        NP_UserData, (IPTR)ks,
        TAG_DONE);
    if (!proc) {
        kfree(ks);
        return ERR_PTR(-ENOMEM);
    }
    Wait(SIGF_SINGLE);
    return ks->ts;
}

bool kthread_should_stop(void)
{
    struct kthread_start *ks = FindTask(NULL)->tc_UserData;
    return ks ? ks->should_stop : false;
}

int kthread_stop(struct task_struct *k)
{
    struct kthread_start *ks = k->task->tc_UserData;

    if (!ks)
        return -EINVAL;
    ks->should_stop = TRUE;
    wake_up_process(k);
    return 0;
}

bool kthread_should_park(void) { return false; }
void kthread_parkme(void) { }
int kthread_park(struct task_struct *k) { return 0; }
void kthread_unpark(struct task_struct *k) { }

/* kthread workers on top of work queues */

void __kthread_work_run(struct work_struct *w)
{
    struct kthread_work *kw = container_of(w, struct kthread_work, work);
    kw->func(kw);
}

struct kthread_worker *kthread_create_worker(unsigned int flags, const char namefmt[], ...)
{
    struct kthread_worker *worker = kzalloc(sizeof(*worker), GFP_KERNEL);
    char name[32];
    va_list ap;

    if (!worker)
        return ERR_PTR(-ENOMEM);
    va_start(ap, namefmt);
    vsnprintf(name, sizeof(name), namefmt, ap);
    va_end(ap);
    worker->wq = alloc_workqueue("%s", 0, 1, name);
    if (!worker->wq) {
        kfree(worker);
        return ERR_PTR(-ENOMEM);
    }
    return worker;
}

bool kthread_queue_work(struct kthread_worker *worker, struct kthread_work *work)
{
    return queue_work(worker->wq, &work->work);
}

void kthread_flush_worker(struct kthread_worker *worker)
{
    flush_workqueue(worker->wq);
}

void kthread_destroy_worker(struct kthread_worker *worker)
{
    destroy_workqueue(worker->wq);
    kfree(worker);
}

bool kthread_cancel_work_sync(struct kthread_work *work)
{
    return cancel_work_sync(&work->work);
}

void kthread_flush_work(struct kthread_work *work)
{
    flush_work(&work->work);
}
