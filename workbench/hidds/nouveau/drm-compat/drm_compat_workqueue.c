/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 1

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <devices/timer.h>

#include <drm-compat/drm_compat_funcs.h>

struct workqueue_message
{
    struct Message  msg;
    struct work_struct *work;
};

static struct MsgPort       *workqueue_port = NULL;
static struct Process       *workqueue_proc = NULL;
static struct Task          *parent_task = NULL;

static struct MsgPort       *timer_port = NULL;
static struct Process       *timer_proc = NULL;

extern struct timerequest *io;

#define WORK_FINISHED   0 /* OK TO SCHEDULE */
#define WORK_SCHEDULED  1 /* NOT OK TO SCHEDULE */
#define WORK_EXECUTING  2 /* OK TO SCHEDULE */

static void worker_main(void)
{
    struct workqueue_message *wmsg;

    D(bug("[Nouveau] WorkQueue worker started\n"));

    workqueue_port = CreateMsgPort();
    if (!workqueue_port)
    {
        D(bug("[Nouveau] WorkQueue: Failed to create message port\n"));
        return;
    }

    D(bug("[Nouveau] WorkQueue: Message port created, signaling parent\n"));

    Signal(parent_task, SIGF_SINGLE);

    for (;;)
    {
        WaitPort(workqueue_port);

        while ((wmsg = (struct workqueue_message *)GetMsg(workqueue_port)))
        {
            D(bug("[Nouveau] WorkQueue: Executing work %p func %p\n", wmsg->work, wmsg->work->func));

            /* work->state == WORK_SCHEDULED */
            wmsg->work->state = WORK_EXECUTING;
            wmsg->work->func(wmsg->work);
            /* func() could have re-scheduled work */
            if (wmsg->work->state != WORK_SCHEDULED)
                wmsg->work->state = WORK_FINISHED;

            FreeVec(wmsg);
        }
    }
}

static void timer_main(void)
{
    struct Message *msg;

    D(bug("[Nouveau] TimerProcess started\n"));

    timer_port = CreateMsgPort();
    if (!timer_port)
    {
        D(bug("[Nouveau] TimerProcess: Failed to create message port\n"));
        return;
    }

    D(bug("[Nouveau] TimerProcess: Message port created, signaling parent\n"));

    Signal(parent_task, SIGF_SINGLE);

    for (;;)
    {
        Wait(1UL << timer_port->mp_SigBit);

        while ((msg = GetMsg(timer_port)))
        {
            struct timerequest *treq = (struct timerequest *)msg;
            struct delayed_work *dwork =
                container_of(treq, struct delayed_work, timer_req);

            D(bug("[Nouveau] TimerProcess: Timer fired for dwork %p\n", dwork));

            /* Forward the work to the work queue */
            queue_work(NULL, &dwork->work);

            dwork->timer_pending = FALSE;
        }
    }
}

BOOL workqueue_init()
{
    D(bug("[Nouveau] WorkQueue: Initializing\n"));

    parent_task = FindTask(NULL);

    workqueue_proc = CreateNewProcTags(
        NP_Name,        (IPTR)"NouveauWorkQueue",
        NP_Priority,    10,
        NP_Entry,       (IPTR)worker_main,
        NP_StackSize,   64 * 1024,
        TAG_DONE);

    if (!workqueue_proc)
    {
        D(bug("[Nouveau] WorkQueue: Failed to create worker process\n"));
        parent_task = NULL;
        return FALSE;
    }

    Wait(SIGF_SINGLE);

    timer_proc = CreateNewProcTags(
        NP_Name,        (IPTR)"NouveauDelayedWorkTimer",
        NP_Priority,    10,
        NP_Entry,       (IPTR)timer_main,
        NP_StackSize,   64 * 1024,
        TAG_DONE);

    if (!timer_proc)
    {
        D(bug("[Nouveau] WorkQueue: Failed to create timer process\n"));
        return FALSE;
    }

    Wait(SIGF_SINGLE);

    D(bug("[Nouveau] WorkQueue: Initialized successfully\n"));
    return TRUE;
}

bool queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
    struct workqueue_message *wmsg;

    D(bug("[Nouveau] queue_work called\n"));

    if (!work)
        return FALSE;

    if (work->state == WORK_SCHEDULED)
        return FALSE;

    wmsg = (struct workqueue_message *)AllocVec(sizeof(struct workqueue_message), MEMF_PUBLIC | MEMF_CLEAR);
    if (!wmsg)
    {
        D(bug("[Nouveau] WorkQueue: Failed to allocate message\n"));
        return FALSE;
    }

    wmsg->msg.mn_Length = sizeof(struct workqueue_message);
    wmsg->work = work;
    work->state = WORK_SCHEDULED;

    D(bug("[Nouveau] WorkQueue: Queuing work %p\n", work));

    PutMsg(workqueue_port, &wmsg->msg);

    return TRUE;
}

bool schedule_work(struct work_struct *work)
{
    return queue_work(NULL, work);
}

bool flush_work(struct work_struct *work)
{
    if (work->state == WORK_FINISHED)
        return FALSE;

    while(work->state != WORK_FINISHED)
        udelay(125);

    return TRUE;
}

bool schedule_delayed_work(struct delayed_work *dwork, unsigned long delay)
{
    D(bug("[Nouveau] schedule_delayed_work dwork=%p delay=%lu jiffies\n", dwork, delay));

    if (!dwork)
        return FALSE;

    if (dwork->timer_pending || dwork->work.state == WORK_SCHEDULED)
        return FALSE;

    if (delay == 0)
    {
        /* Zero delay: schedule immediately */
        return queue_work(NULL, &dwork->work);
    }

    unsigned int usecs = jiffies_to_usecs(delay);

    /* Set up the timer request */
    dwork->timer_req = *io;
    dwork->timer_req.tr_node.io_Message.mn_Node.ln_Type  = NT_REPLYMSG;
    dwork->timer_req.tr_node.io_Message.mn_ReplyPort     = timer_port;
    dwork->timer_req.tr_node.io_Command                  = TR_ADDREQUEST;
    dwork->timer_req.tr_time.tv_secs                     = usecs / 1000000;
    dwork->timer_req.tr_time.tv_micro                    = usecs % 1000000;

    dwork->timer_pending = TRUE;

    /* Submit the timer request asynchronously */
    SendIO((struct IORequest *)&dwork->timer_req);

    return TRUE;
}

bool cancel_delayed_work_sync(struct delayed_work *dwork)
{
    bool ret;

    D(bug("[Nouveau] cancel_delayed_work_sync dwork=%p\n", dwork));

    if (!dwork)
        return FALSE;

    ret = FALSE;

    if (dwork->timer_pending)
    {
        AbortIO((struct IORequest *)&dwork->timer_req);
        WaitIO((struct IORequest *)&dwork->timer_req);
        dwork->timer_pending = FALSE;
        ret = TRUE;
    }

    /* Ensure any in-flight work completes */
    if (dwork->work.state == WORK_SCHEDULED)
    {
        flush_work(&dwork->work);
        ret = TRUE;
    }

    return ret;
}