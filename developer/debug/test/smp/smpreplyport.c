/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for several replies arriving on one reply port.
          Workers pinned to each core keep six timer.device requests
          outstanding on one port, with staggered delays, and WaitIO()
          them in turn. The replies come from the timer interrupt on
          another core while WaitIO() takes earlier ones off the port's
          list, so ReplyMsg() and WaitIO() meet on the same list. After
          each round the port must be empty and every request must have
          completed.

          ReplyMsg() used to mark a message NT_REPLYMSG before queueing
          it; WaitIO() on another core took the type as the sign that the
          message was on the list and removed a node that was not linked
          yet, and the reply then stayed on the port.
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
#define PER_CPU         2
#define MAX_WORKERS     (4 * PER_CPU)
#define BATCH           6
#define ROUNDS          300
#define WAIT_TICKS      3000        /* 60 s bound */

struct PortWorker
{
    volatile ULONG  w_Done;
    volatile ULONG  w_Completed;
    volatile ULONG  w_Errors;
    volatile ULONG  w_Leftover;
};

static struct PortWorker *myworker(void)
{
    return (struct PortWorker *)FindTask(NULL)->tc_UserData;
}

static struct Task *spawn_pinned(const char *name, APTR pc, int cpu,
                                 struct PortWorker *w)
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

static void PortWorker(void)
{
    struct PortWorker *w = myworker();
    struct MsgPort *port = CreateMsgPort();
    struct timerequest *tr[BATCH];
    ULONG round;
    int k, opened = 0;

    memset(tr, 0, sizeof(tr));
    if (!port)
        goto fail;
    for (k = 0; k < BATCH; k++)
    {
        tr[k] = (struct timerequest *)CreateIORequest(port, sizeof(struct timerequest));
        if (!tr[k] || OpenDevice("timer.device", UNIT_MICROHZ,
                                 (struct IORequest *)tr[k], 0))
            goto fail;
        opened++;
    }

    for (round = 0; round < ROUNDS; round++)
    {
        /* Staggered, so replies arrive while earlier ones are taken off */
        for (k = 0; k < BATCH; k++)
        {
            tr[k]->tr_node.io_Command = TR_ADDREQUEST;
            tr[k]->tr_time.tv_secs    = 0;
            tr[k]->tr_time.tv_micro   = 300 + 400 * k;
            SendIO((struct IORequest *)tr[k]);
        }
        for (k = 0; k < BATCH; k++)
        {
            if (WaitIO((struct IORequest *)tr[k]))
                w->w_Errors++;
            else
                w->w_Completed++;
        }
        /* Every reply was taken off by its WaitIO(): nothing may be left */
        while (GetMsg(port))
            w->w_Leftover++;
    }

    for (k = 0; k < BATCH; k++)
    {
        CloseDevice((struct IORequest *)tr[k]);
        DeleteIORequest((struct IORequest *)tr[k]);
    }
    DeleteMsgPort(port);
    w->w_Done = 1;
    Wait(0);

fail:
    w->w_Errors++;
    for (k = 0; k < BATCH; k++)
    {
        if (k < opened)
            CloseDevice((struct IORequest *)tr[k]);
        if (tr[k])
            DeleteIORequest((struct IORequest *)tr[k]);
    }
    if (port)
        DeleteMsgPort(port);
    w->w_Done = 1;
    Wait(0);
}

int main(void)
{
    struct PortWorker w[MAX_WORKERS];
    struct Task *t[MAX_WORKERS];
    int n, k, tick, ok = 1, created = 0;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();

    n = (g_NumCPUs < 4 ? g_NumCPUs : 4) * PER_CPU;
    bug("[smpreplyport] start: cpus=%ld, %d workers x %d rounds of %d "
        "requests on one reply port\n", (LONG)g_NumCPUs, n, ROUNDS, BATCH);

    if (g_NumCPUs < 2)
    {
        bug("[smpreplyport] DONE: 0 PASS, 0 FAIL, 1 INVALID (needs two CPUs)\n");
        return RETURN_OK;
    }

    memset(w, 0, sizeof(w));
    for (k = 0; k < n; k++)
    {
        t[k] = spawn_pinned("smpreplyport.worker", PortWorker, k / PER_CPU, &w[k]);
        if (!t[k])
            break;
        created++;
    }

    for (tick = 0; tick < WAIT_TICKS; tick++)
    {
        int done = 0;

        for (k = 0; k < created; k++)
            if (w[k].w_Done)
                done++;
        if (done == created)
            break;
        Delay(1);
    }

    Forbid();
    for (k = 0; k < created; k++)
        RemTask(t[k]);
    Permit();

    if (created < n)
    {
        bug("[smpreplyport] DONE: 0 PASS, 0 FAIL, 1 INVALID (task create failed)\n");
        return RETURN_OK;
    }
    if (tick == WAIT_TICKS)
    {
        bug("[smpreplyport] *** TIMEOUT *** (workers stuck)\n");
        ok = 0;
    }

    for (k = 0; k < n; k++)
    {
        if (w[k].w_Errors || w[k].w_Leftover ||
            w[k].w_Completed != (ULONG)ROUNDS * BATCH)
        {
            bug("[smpreplyport] *** FAIL *** worker %d (cpu %d): %lu completed, "
                "%lu errors, %lu messages left on the port\n", k, k / PER_CPU,
                (unsigned long)w[k].w_Completed, (unsigned long)w[k].w_Errors,
                (unsigned long)w[k].w_Leftover);
            ok = 0;
        }
    }

    bug("[smpreplyport] DONE: %d PASS, %d FAIL, 0 INVALID\n", ok, !ok);
    return ok ? RETURN_OK : RETURN_FAIL;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
