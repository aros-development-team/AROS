/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/ports.h>

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

/*
 * A workqueue is one worker process with a message port. Queued work
 * items are linked on the queue's list under Disable(); the worker pops
 * and runs them. Delayed work parks on a kernel timer whose expiry queues
 * the item. The system queues share one worker.
 */

enum {
    WORK_IDLE = 0,
    WORK_QUEUED,
    WORK_RUNNING,
};

struct workqueue_struct {
    char name[32];
    struct MinList items;
    struct Process *worker;
    struct Task *creator;
    ULONG sigbit;
    volatile struct work_struct *running;
    volatile ULONG generation;
    volatile ULONG done_generation;
    struct SignalSemaphore flush_lock;
    volatile BOOL stopping;
};

struct workqueue_struct *system_wq = NULL;
struct workqueue_struct *system_highpri_wq = NULL;
struct workqueue_struct *system_long_wq = NULL;
struct workqueue_struct *system_unbound_wq = NULL;
struct workqueue_struct *system_freezable_wq = NULL;
struct workqueue_struct *system_power_efficient_wq = NULL;

static void worker_main(void)
{
    struct Task *self = FindTask(NULL);
    struct workqueue_struct *wq = self->tc_UserData;

    wq->sigbit = AllocSignal(-1);
    Signal(wq->creator, SIGF_SINGLE);
    if (wq->sigbit == (ULONG)-1)
        return;

    for (;;) {
        struct work_struct *work;
        struct MinNode *node;

        Wait(1UL << wq->sigbit);
        if (wq->stopping)
            break;

        for (;;) {
            Disable();
            node = (struct MinNode *)REMHEAD(&wq->items);
            work = node ? container_of(node, struct work_struct, node) : NULL;
            if (work) {
                work->state = WORK_RUNNING;
                wq->running = work;
            }
            Enable();
            if (!work)
                break;

            work->func(work);

            Disable();
            /* the function may have queued itself again */
            if (work->state == WORK_RUNNING)
                work->state = WORK_IDLE;
            wq->running = NULL;
            wq->done_generation = wq->generation;
            Enable();
        }
    }
    FreeSignal(wq->sigbit);
}

struct workqueue_struct *alloc_workqueue(const char *fmt, unsigned int flags, int max_active, ...)
{
    struct workqueue_struct *wq;
    va_list ap;

    wq = kzalloc(sizeof(*wq), GFP_KERNEL);
    if (!wq)
        return NULL;

    va_start(ap, max_active);
    vsnprintf(wq->name, sizeof(wq->name), fmt, ap);
    va_end(ap);

    NEWLIST(&wq->items);
    InitSemaphore(&wq->flush_lock);
    wq->creator = FindTask(NULL);
    wq->sigbit = (ULONG)-1;

    wq->worker = CreateNewProcTags(
        NP_Name, (IPTR)wq->name,
        /*
         * Above every system task that may wait on this work: the display
         * commit polls from input.device (priority 20), and a worker below
         * that priority is starved for the whole wait on strict-priority
         * scheduling. The workers sleep unless work is queued.
         */
        NP_Priority, (flags & WQ_HIGHPRI) ? 22 : 21,
        NP_Entry, (IPTR)worker_main,
        NP_StackSize, 256 * 1024,
        NP_UserData, (IPTR)wq,
        TAG_DONE);
    if (!wq->worker) {
        kfree(wq);
        return NULL;
    }
    Wait(SIGF_SINGLE);
    if (wq->sigbit == (ULONG)-1) {
        kfree(wq);
        return NULL;
    }
    return wq;
}

void destroy_workqueue(struct workqueue_struct *wq)
{
    if (!wq)
        return;
    flush_workqueue(wq);
    wq->stopping = TRUE;
    Signal((struct Task *)wq->worker, 1UL << wq->sigbit);
    /* the process frees its own resources; the descriptor stays until the
       driver goes away */
}

void __init_work(struct work_struct *work, work_func_t func)
{
    work->func = func;
    work->state = WORK_IDLE;
    work->wq = NULL;
}

void __init_delayed_work(struct delayed_work *dwork, work_func_t func)
{
    __init_work(&dwork->work, func);
    timer_setup(&dwork->timer, NULL, 0);
}

bool queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
    bool queued = false;

    if (!wq)
        wq = system_wq;
    if (!wq || !work)
        return false;

    Disable();
    if (work->state != WORK_QUEUED) {
        work->state = WORK_QUEUED;
        work->wq = wq;
        ADDTAIL(&wq->items, (struct Node *)&work->node);
        wq->generation++;
        queued = true;
    }
    Enable();

    if (queued)
        Signal((struct Task *)wq->worker, 1UL << wq->sigbit);
    return queued;
}

static void delayed_work_timer_fn(struct timer_list *t)
{
    struct delayed_work *dwork = container_of(t, struct delayed_work, timer);

    queue_work(dwork->work.wq, &dwork->work);
}

bool queue_delayed_work(struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay)
{
    if (!wq)
        wq = system_wq;
    if (!wq)
        return false;

    if (dwork->work.state == WORK_QUEUED || timer_pending(&dwork->timer))
        return false;

    dwork->work.wq = wq;
    if (delay == 0)
        return queue_work(wq, &dwork->work);

    dwork->timer.function = delayed_work_timer_fn;
    mod_timer(&dwork->timer, jiffies + delay);
    return true;
}

bool mod_delayed_work(struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay)
{
    bool was_pending;

    if (!wq)
        wq = system_wq;
    was_pending = del_timer(&dwork->timer);
    dwork->work.wq = wq;
    if (delay == 0) {
        queue_work(wq, &dwork->work);
        return was_pending;
    }
    dwork->timer.function = delayed_work_timer_fn;
    mod_timer(&dwork->timer, jiffies + delay);
    return was_pending;
}

bool queue_rcu_work(struct workqueue_struct *wq, struct rcu_work *rwork)
{
    return queue_work(wq, &rwork->work);
}

bool work_pending(struct work_struct *work)
{
    return work->state == WORK_QUEUED;
}

bool work_busy(struct work_struct *work)
{
    return work->state != WORK_IDLE;
}

/*
 * Waiting for an item means waiting until the worker has moved past it;
 * from inside the worker itself there is nothing to wait for.
 */
bool flush_work(struct work_struct *work)
{
    struct workqueue_struct *wq = work->wq;
    bool waited = false;

    if (!wq)
        return false;
    if (FindTask(NULL) == (struct Task *)wq->worker)
        return false;

    while (work->state != WORK_IDLE) {
        waited = true;
        udelay(100);
    }
    return waited;
}

void flush_workqueue(struct workqueue_struct *wq)
{
    ULONG gen;

    if (!wq)
        return;
    if (FindTask(NULL) == (struct Task *)wq->worker)
        return;

    Disable();
    gen = wq->generation;
    Enable();
    while (!IsListEmpty((struct List *)&wq->items) || wq->running != NULL || (LONG)(wq->done_generation - gen) < 0) {
        if (IsListEmpty((struct List *)&wq->items) && wq->running == NULL)
            break;
        udelay(100);
    }
}

void drain_workqueue(struct workqueue_struct *wq)
{
    flush_workqueue(wq);
}

bool flush_delayed_work(struct delayed_work *dwork)
{
    if (del_timer(&dwork->timer))
        queue_work(dwork->work.wq, &dwork->work);
    return flush_work(&dwork->work);
}

bool flush_rcu_work(struct rcu_work *rwork)
{
    return flush_work(&rwork->work);
}

bool cancel_work(struct work_struct *work)
{
    bool ret = false;

    Disable();
    if (work->state == WORK_QUEUED) {
        REMOVE((struct Node *)&work->node);
        work->state = WORK_IDLE;
        ret = true;
    }
    Enable();
    return ret;
}

bool cancel_work_sync(struct work_struct *work)
{
    bool ret = cancel_work(work);

    flush_work(work);
    return ret;
}

bool cancel_delayed_work(struct delayed_work *dwork)
{
    bool ret = del_timer(&dwork->timer);

    ret |= cancel_work(&dwork->work);
    return ret;
}

bool cancel_delayed_work_sync(struct delayed_work *dwork)
{
    bool ret = cancel_delayed_work(dwork);

    flush_work(&dwork->work);
    return ret;
}

/* tasklets are just work items */

static void tasklet_run(struct work_struct *work)
{
    struct tasklet_struct *t = container_of(work, struct tasklet_struct, work);

    if (t->callback)
        t->callback(t);
    else if (t->func)
        t->func(t->data);
}

void tasklet_setup(struct tasklet_struct *t, void (*callback)(struct tasklet_struct *))
{
    __init_work(&t->work, tasklet_run);
    t->callback = callback;
    t->func = NULL;
}

void tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data)
{
    __init_work(&t->work, tasklet_run);
    t->callback = NULL;
    t->func = func;
    t->data = data;
}

void tasklet_schedule(struct tasklet_struct *t)
{
    queue_work(system_highpri_wq, &t->work);
}

void tasklet_hi_schedule(struct tasklet_struct *t)
{
    queue_work(system_highpri_wq, &t->work);
}

void tasklet_kill(struct tasklet_struct *t)
{
    cancel_work_sync(&t->work);
}

/*
 * The shared queues are created on first use from the caller's context so
 * that the processes exist before any interrupt tries to queue to them.
 */
BOOL nouveau_log_init(void);
void nouveau_compat_time_check(void);

BOOL workqueue_init(void)
{
    if (system_wq)
        return TRUE;

    nouveau_log_init();
    nouveau_compat_time_check();

    system_wq = alloc_workqueue("Nouveau WorkQueue", 0, 0);
    if (!system_wq)
        return FALSE;
    system_highpri_wq = alloc_workqueue("Nouveau WorkQueue HiPri", WQ_HIGHPRI, 0);
    if (!system_highpri_wq)
        system_highpri_wq = system_wq;
    system_long_wq = system_wq;
    system_unbound_wq = system_wq;
    system_freezable_wq = system_wq;
    system_power_efficient_wq = system_wq;
    return TRUE;
}
