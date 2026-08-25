/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for device I/O round trips. One worker per core
          drives its own timer.device requests with DoIO, then with a
          SendIO/AbortIO/WaitIO mix. The completions arrive from the
          timer interrupt on another core, so the reply path (PutMsg,
          Signal, port arbitration) is exercised across cores.
*/

#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/timer.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
static int g_NumCPUs = 1;

#define STACKSIZE       16384
#define WAIT_TICKS      750         /* 15s bound: many timer round trips */
#define DOIO_ITERS      200
#define SENDIO_ITERS    100

struct IOWorker
{
    volatile ULONG  w_Started;
    volatile ULONG  w_Done;
    volatile ULONG  w_Errors;
    volatile ULONG  w_Completed;
    volatile ULONG  w_Aborted;
};

static struct IOWorker *myworker(void)
{
    return (struct IOWorker *)FindTask(NULL)->tc_UserData;
}

static struct Task *spawn_pinned(const char *name, APTR pc, int cpu,
                                 struct IOWorker *w)
{
    cpumask_t *mask = KrnAllocCPUMask();

    if (!mask)
        return NULL;
    KrnClearCPUMask(mask);
    KrnGetCPUMask(cpu % g_NumCPUs, mask);
    return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                         TASKTAG_PRI,       0,
                         TASKTAG_PC,        (IPTR)pc,
                         TASKTAG_STACKSIZE, STACKSIZE,
                         TASKTAG_USERDATA,  (IPTR)w,
                         TASKTAG_AFFINITY,  (IPTR)mask,
                         TAG_DONE);
}

static int wait_all(const char *phase, struct IOWorker *w, int n)
{
    int i, t;

    for (t = 0; t < WAIT_TICKS; t++)
    {
        int done = 0;

        for (i = 0; i < n; i++)
            if (w[i].w_Done)
                done++;
        if (done == n)
            return 1;
        Delay(1);
    }
    bug("[smpdoio] %s: *** TIMEOUT *** (workers stuck)\n", phase);
    return 0;
}

/* Open a private timer.device unit for this task. */
static struct timerequest *open_timer(struct MsgPort **portp)
{
    struct MsgPort *port;
    struct timerequest *tr;

    port = CreateMsgPort();
    if (!port)
        return NULL;
    tr = (struct timerequest *)CreateIORequest(port, sizeof(struct timerequest));
    if (!tr)
    {
        DeleteMsgPort(port);
        return NULL;
    }
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0))
    {
        DeleteIORequest((struct IORequest *)tr);
        DeleteMsgPort(port);
        return NULL;
    }
    *portp = port;
    return tr;
}

static void close_timer(struct timerequest *tr, struct MsgPort *port)
{
    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(port);
}

/******************************************************************************/
/*  Phase 1: synchronous DoIO round trips from every core                     */
/******************************************************************************/

static void DoIOWorker(void)
{
    struct IOWorker *w = myworker();
    struct MsgPort *port;
    struct timerequest *tr;
    ULONG i;

    tr = open_timer(&port);
    if (!tr)
    {
        w->w_Errors = 1;
        w->w_Started = 1;
        w->w_Done = 1;
        Wait(0);
    }

    w->w_Started = 1;

    for (i = 0; i < DOIO_ITERS; i++)
    {
        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs    = 0;
        tr->tr_time.tv_micro   = 1000;
        if (DoIO((struct IORequest *)tr))
            w->w_Errors++;
        else
            w->w_Completed++;
    }

    close_timer(tr, port);
    w->w_Done = 1;
    Wait(0);
}

static int TestDoIO(void)
{
    struct IOWorker w[4];
    struct Task *t[4];
    int n = g_NumCPUs < 4 ? g_NumCPUs : 4, k, ok = 1;

    bug("[smpdoio] phase 1: %d workers x %d DoIO timer round trips...\n",
        n, DOIO_ITERS);

    memset(w, 0, sizeof(w));
    for (k = 0; k < n; k++)
    {
        t[k] = spawn_pinned("smpdoio.doio", DoIOWorker, k, &w[k]);
        if (!t[k])
        {
            bug("[smpdoio] phase 1: INVALID (task create failed)\n");
            return -1;
        }
    }

    if (!wait_all("phase 1", w, n))
        ok = 0;

    Forbid();
    for (k = 0; k < n; k++)
        RemTask(t[k]);
    Permit();

    if (!ok)
        return 0;

    for (k = 0; k < n; k++)
    {
        if (w[k].w_Errors || w[k].w_Completed != DOIO_ITERS)
        {
            bug("[smpdoio] phase 1: *** FAIL *** worker %d: %lu done, %lu errors\n",
                k, (unsigned long)w[k].w_Completed, (unsigned long)w[k].w_Errors);
            return 0;
        }
    }
    bug("[smpdoio] phase 1: OK\n");
    return 1;
}

/******************************************************************************/
/*  Phase 2: SendIO with AbortIO on every second request                      */
/******************************************************************************/

static void SendIOWorker(void)
{
    struct IOWorker *w = myworker();
    struct MsgPort *port;
    struct timerequest *tr;
    ULONG i;

    tr = open_timer(&port);
    if (!tr)
    {
        w->w_Errors = 1;
        w->w_Started = 1;
        w->w_Done = 1;
        Wait(0);
    }

    w->w_Started = 1;

    for (i = 0; i < SENDIO_ITERS; i++)
    {
        BYTE err;

        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs    = 0;
        tr->tr_time.tv_micro   = (i & 1) ? 500000 : 500;
        SendIO((struct IORequest *)tr);

        /* Abort the long ones so the phase stays fast; the short ones
         * usually complete first and the abort must cope with that. */
        if (i & 1)
            AbortIO((struct IORequest *)tr);

        err = WaitIO((struct IORequest *)tr);
        if (err == IOERR_ABORTED)
            w->w_Aborted++;
        else if (err == 0)
            w->w_Completed++;
        else
            w->w_Errors++;
    }

    close_timer(tr, port);
    w->w_Done = 1;
    Wait(0);
}

static int TestSendIO(void)
{
    struct IOWorker w[4];
    struct Task *t[4];
    int n = g_NumCPUs < 4 ? g_NumCPUs : 4, k, ok = 1;

    bug("[smpdoio] phase 2: %d workers x %d SendIO/AbortIO/WaitIO...\n",
        n, SENDIO_ITERS);

    memset(w, 0, sizeof(w));
    for (k = 0; k < n; k++)
    {
        t[k] = spawn_pinned("smpdoio.sendio", SendIOWorker, k, &w[k]);
        if (!t[k])
        {
            bug("[smpdoio] phase 2: INVALID (task create failed)\n");
            return -1;
        }
    }

    if (!wait_all("phase 2", w, n))
        ok = 0;

    Forbid();
    for (k = 0; k < n; k++)
        RemTask(t[k]);
    Permit();

    if (!ok)
        return 0;

    for (k = 0; k < n; k++)
    {
        bug("[smpdoio] phase 2: worker %d: %lu completed, %lu aborted, %lu errors\n",
            k, (unsigned long)w[k].w_Completed, (unsigned long)w[k].w_Aborted,
            (unsigned long)w[k].w_Errors);
        if (w[k].w_Errors ||
            (w[k].w_Completed + w[k].w_Aborted) != SENDIO_ITERS)
        {
            bug("[smpdoio] phase 2: *** FAIL *** worker %d lost requests\n", k);
            return 0;
        }
    }
    bug("[smpdoio] phase 2: OK\n");
    return 1;
}

int main(void)
{
    int pass = 0, fail = 0, inval = 0;
    int r;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();

    bug("[smpdoio] start: cpus=%ld\n", (LONG)g_NumCPUs);

    r = TestDoIO();    if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestSendIO();  if (r > 0) pass++; else if (!r) fail++; else inval++;

    bug("[smpdoio] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, inval);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
