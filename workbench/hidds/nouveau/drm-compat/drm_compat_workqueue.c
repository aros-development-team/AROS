/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 1

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <exec/ports.h>

#include <drm-compat/drm_compat_funcs.h>

struct workqueue_message
{
    struct Message  msg;
    struct work_struct *work;
};

static struct MsgPort       *workqueue_port = NULL;
static struct Process       *workqueue_proc = NULL;
static struct Task          *parent_task = NULL;

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
            D(bug("[Nouveau] WorkQueue: Executing work %p func %p\n",
                wmsg->work, wmsg->work->func));

            wmsg->work->func(wmsg->work);

            FreeVec(wmsg);
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

    parent_task = NULL;

    D(bug("[Nouveau] WorkQueue: Initialized successfully\n"));
    return TRUE;
}

bool queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
    struct workqueue_message *wmsg;

    D(bug("[Nouveau] queue_work called\n"));

    if (!work)
        return FALSE;

    wmsg = (struct workqueue_message *)AllocVec(sizeof(struct workqueue_message), MEMF_PUBLIC | MEMF_CLEAR);
    if (!wmsg)
    {
        D(bug("[Nouveau] WorkQueue: Failed to allocate message\n"));
        return FALSE;
    }

    wmsg->msg.mn_Length = sizeof(struct workqueue_message);
    wmsg->work = work;

    D(bug("[Nouveau] WorkQueue: Queuing work %p\n", work));

    PutMsg(workqueue_port, &wmsg->msg);

    return TRUE;
}
