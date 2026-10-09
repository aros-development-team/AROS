/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP stress for timer.device. Per core a pthread_cond_timedwait-,
          an AROSTCP tsleep- and a nanosleep-style worker, plus wakers on
          other cores. Reports workers that stop making progress.
          Usage: SMP-Timer [seconds]
*/

#include <aros/config.h>

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

#include <stdlib.h>
#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
static int g_NumCPUs = 1;
static volatile BOOL g_StopWorkers, g_StopWakers;

#define STACKSIZE   32768
#define MAXCPU      8
#define STUCK_SECS  5

enum { W_COND, W_TSLEEP, W_SLEEP, W_KINDS };
static const char *kindname[W_KINDS] = { "cond", "tsleep", "sleep" };

struct Worker
{
    struct Task    *w_Task;
    struct Task    *w_Waker;
    int             w_Kind;
    int             w_CPU;
    BYTE            w_WakeSig;
    volatile ULONG  w_Started;
    volatile ULONG  w_Done;
    volatile ULONG  w_Iter;
    volatile ULONG  w_Timer;    /* completed by the timer */
    volatile ULONG  w_Early;    /* woken first, request aborted */
    volatile ULONG  w_Errors;
    ULONG           w_Seed;
};

static ULONG rnd(ULONG *s)
{
    *s ^= *s << 13;
    *s ^= *s >> 17;
    *s ^= *s << 5;
    return *s;
}

static struct Worker *me(void)
{
    return (struct Worker *)FindTask(NULL)->tc_UserData;
}

static struct Task *spawn_pinned(const char *name, APTR pc, int cpu, APTR data)
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
                         TASKTAG_USERDATA,  (IPTR)data,
                         TASKTAG_AFFINITY,  (IPTR)mask,
                         TAG_DONE);
}

static void finish(struct Worker *w)
{
    w->w_Done = 1;
    Wait(0);
}

/* pthread_cond_timedwait(): everything on the stack, opened per wait */
static void CondWorker(void)
{
    struct Worker *w = me();
    ULONG wake;

    w->w_WakeSig = AllocSignal(-1);
    wake = 1UL << w->w_WakeSig;

    w->w_Started = 1;
    while (!g_StopWorkers)
    {
        struct MsgPort port;
        struct timerequest tr;
        ULONG sigs;

        memset(&port, 0, sizeof(port));
        port.mp_Node.ln_Type = NT_MSGPORT;
        port.mp_Flags = PA_SIGNAL;
        port.mp_SigTask = FindTask(NULL);
        port.mp_SigBit = AllocSignal(-1);
        NEWLIST(&port.mp_MsgList);
        if ((BYTE)port.mp_SigBit == -1)
        {
            w->w_Errors++;
            break;
        }
        memset(&tr, 0, sizeof(tr));
        tr.tr_node.io_Message.mn_ReplyPort = &port;
        tr.tr_node.io_Message.mn_Length = sizeof(tr);
        if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)&tr, 0))
        {
            FreeSignal(port.mp_SigBit);
            w->w_Errors++;
            break;
        }

        tr.tr_node.io_Command = TR_ADDREQUEST;
        tr.tr_time.tv_secs = 0;
        tr.tr_time.tv_micro = 100 + rnd(&w->w_Seed) % 3000;
        SendIO((struct IORequest *)&tr);

        sigs = Wait((1UL << port.mp_SigBit) | wake);
        (void)sigs;
        if (!CheckIO((struct IORequest *)&tr))
        {
            AbortIO((struct IORequest *)&tr);
            WaitIO((struct IORequest *)&tr);
            w->w_Early++;
        }
        else
            w->w_Timer++;

        CloseDevice((struct IORequest *)&tr);
        FreeSignal(port.mp_SigBit);
        w->w_Iter++;
    }
    finish(w);
}

/* AROSTCP's tsleep(): one VBLANK request, reused, aborted or collected */
static void TsleepWorker(void)
{
    struct Worker *w = me();
    ULONG wake;
    struct MsgPort *port;
    struct timerequest *tr;

    w->w_WakeSig = AllocSignal(-1);
    wake = 1UL << w->w_WakeSig;
    port = CreateMsgPort();
    tr = port ? (struct timerequest *)CreateIORequest(port, sizeof(*tr)) : NULL;

    if (!tr || OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0))
    {
        w->w_Errors++;
        w->w_Started = 1;
        finish(w);
        return;
    }
    tr->tr_node.io_Message.mn_Node.ln_Type = NT_UNKNOWN;

    w->w_Started = 1;
    while (!g_StopWorkers)
    {
        ULONG sigs;

        if (tr->tr_node.io_Message.mn_Node.ln_Type != NT_UNKNOWN)
        {
            if (tr->tr_node.io_Message.mn_Node.ln_Type != NT_REPLYMSG)
                AbortIO((struct IORequest *)tr);
            WaitIO((struct IORequest *)tr);
            tr->tr_node.io_Message.mn_Node.ln_Type = NT_UNKNOWN;
            w->w_Early++;
        }

        tr->tr_node.io_Command = TR_ADDREQUEST;
        tr->tr_time.tv_secs = 0;
        tr->tr_time.tv_micro = rnd(&w->w_Seed) % 40000;
        BeginIO((struct IORequest *)tr);

        sigs = Wait((1UL << port->mp_SigBit) | wake);
        if ((sigs & (1UL << port->mp_SigBit)) && GetMsg(port) == (struct Message *)tr)
        {
            tr->tr_node.io_Message.mn_Node.ln_Type = NT_UNKNOWN;
            w->w_Timer++;
        }
        w->w_Iter++;
    }
    if (tr->tr_node.io_Message.mn_Node.ln_Type != NT_UNKNOWN)
    {
        if (tr->tr_node.io_Message.mn_Node.ln_Type != NT_REPLYMSG)
            AbortIO((struct IORequest *)tr);
        WaitIO((struct IORequest *)tr);
    }
    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(port);
    finish(w);
}

/* nanosleep(): everything created and DoIO()ed per call */
static void SleepWorker(void)
{
    struct Worker *w = me();

    w->w_Started = 1;
    while (!g_StopWorkers)
    {
        struct MsgPort *port = CreateMsgPort();
        struct timerequest *tr = port ? (struct timerequest *)CreateIORequest(port, sizeof(*tr)) : NULL;

        if (tr && !OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0))
        {
            tr->tr_node.io_Command = TR_ADDREQUEST;
            tr->tr_time.tv_secs = 0;
            tr->tr_time.tv_micro = 50 + rnd(&w->w_Seed) % 2000;
            if (DoIO((struct IORequest *)tr))
                w->w_Errors++;
            else
                w->w_Timer++;
            CloseDevice((struct IORequest *)tr);
        }
        else
            w->w_Errors++;
        DeleteIORequest((struct IORequest *)tr);
        DeleteMsgPort(port);
        w->w_Iter++;
    }
    finish(w);
}

/* Wakes its worker at random moments, from another core (cond_signal) */
static void Waker(void)
{
    struct Worker *w = (struct Worker *)FindTask(NULL)->tc_UserData;
    ULONG seed = w->w_Seed ^ 0x9e3779b9;

    while (!g_StopWakers)
    {
        volatile ULONG spin = rnd(&seed) % 200000;

        Signal(w->w_Task, 1UL << w->w_WakeSig);
        while (spin--)
            ;
    }
    Wait(0);
}

int main(void)
{
    static struct Worker workers[MAXCPU * W_KINDS];
    static const APTR pcs[W_KINDS] = { CondWorker, TsleepWorker, SleepWorker };
    IPTR args[1] = { 0 };
    struct RDArgs *rda;
    LONG seconds = 60, t;
    int n, i, cpu, kind, stuck = 0, errors = 0;
    ULONG last[MAXCPU * W_KINDS], idle[MAXCPU * W_KINDS];

    if ((rda = ReadArgs("SECONDS/N", args, NULL)))
    {
        if (args[0])
            seconds = *(LONG *)args[0];
        FreeArgs(rda);
    }

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();
    if (g_NumCPUs > MAXCPU)
        g_NumCPUs = MAXCPU;
    n = g_NumCPUs * W_KINDS;

    bug("[smptimer] start: %d cpus, %d workers, %ld s\n", g_NumCPUs, n, (long)seconds);

    for (i = 0, cpu = 0; cpu < g_NumCPUs; cpu++)
    {
        for (kind = 0; kind < W_KINDS; kind++, i++)
        {
            struct Worker *w = &workers[i];

            w->w_Kind = kind;
            w->w_CPU = cpu;
            w->w_Seed = 0x12345 + i * 7919;
            w->w_WakeSig = -1;      /* the worker allocates its own */
            w->w_Task = spawn_pinned("smptimer.worker", pcs[kind], cpu, w);
            if (!w->w_Task)
            {
                bug("[smptimer] INVALID: task create failed\n");
                return RETURN_FAIL;
            }
            while (!w->w_Started)
                Delay(1);
            /* SleepWorker blocks in DoIO and needs no waker */
            if (kind != W_SLEEP)
            {
                w->w_Waker = spawn_pinned("smptimer.waker", Waker, cpu + 1 + kind, w);
                if (!w->w_Waker)
                {
                    bug("[smptimer] INVALID: task create failed\n");
                    return RETURN_FAIL;
                }
            }
            last[i] = 0;
            idle[i] = 0;
        }
    }

    for (t = 1; t <= seconds; t++)
    {
        Delay(50);
        for (i = 0; i < n; i++)
        {
            ULONG it = workers[i].w_Iter;

            if (it == last[i])
            {
                if (++idle[i] == STUCK_SECS)
                {
                    bug("[smptimer] *** STUCK *** %s worker on cpu %d: no progress for %d s at iteration %lu\n",
                        kindname[workers[i].w_Kind], workers[i].w_CPU, STUCK_SECS, (unsigned long)it);
                    stuck++;
                }
            }
            else
                idle[i] = 0;
            last[i] = it;
        }
        if (t % 10 == 0 || t == seconds)
        {
            ULONG tot[W_KINDS] = { 0 };
            for (i = 0; i < n; i++)
                tot[workers[i].w_Kind] += workers[i].w_Iter;
            bug("[smptimer] %lds: cond %lu, tsleep %lu, sleep %lu iterations\n",
                (long)t, (unsigned long)tot[W_COND], (unsigned long)tot[W_TSLEEP], (unsigned long)tot[W_SLEEP]);
        }
    }

    /* Wakers first: they signal the workers */
    g_StopWakers = TRUE;
    Delay(10);
    g_StopWorkers = TRUE;
    for (i = 0; i < n; i++)
    {
        /* A worker parked in Wait() needs one more wake-up */
        for (t = 0; t < 100 && !workers[i].w_Done; t++)
        {
            if (workers[i].w_WakeSig != -1)
                Signal(workers[i].w_Task, 1UL << workers[i].w_WakeSig);
            Delay(1);
        }
        if (!workers[i].w_Done)
        {
            bug("[smptimer] *** STUCK *** %s worker on cpu %d did not finish\n",
                kindname[workers[i].w_Kind], workers[i].w_CPU);
            stuck++;
        }
    }

    Forbid();
    for (i = 0; i < n; i++)
    {
        if (workers[i].w_Waker)
            RemTask(workers[i].w_Waker);
        if (workers[i].w_Done)
            RemTask(workers[i].w_Task);
    }
    Permit();

    for (i = 0; i < n; i++)
    {
        struct Worker *w = &workers[i];

        bug("[smptimer] %-6s cpu %d: %lu iterations, %lu by timer, %lu woken early, %lu errors\n",
            kindname[w->w_Kind], w->w_CPU, (unsigned long)w->w_Iter, (unsigned long)w->w_Timer,
            (unsigned long)w->w_Early, (unsigned long)w->w_Errors);
        errors += w->w_Errors;
    }
    bug("[smptimer] DONE: %s (%d stuck, %d errors)\n", (stuck || errors) ? "FAIL" : "PASS", stuck, errors);
    return (stuck || errors) ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
