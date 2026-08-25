/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for the semaphore paths the other tests skip:
          ObtainSemaphoreList against single obtainers, the asynchronous
          Procure/Vacate protocol against blocking obtainers, and shared
          readers holding an invariant against an exclusive writer.

          Workers are pinned across cores, so these paths race for real
          instead of being serialised by a single CPU.
*/

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#define DEBUG 1
#include <aros/debug.h>

APTR KernelBase;
static int g_NumCPUs = 1;

#define STACKSIZE       16384
#define WAIT_TICKS      500         /* 10s bound for Delay-based waits */
#define LIST_SEMS       4
#define LIST_ITERS      2000
#define SINGLE_ITERS    4000
#define PROCURE_ITERS   2000
#define OBTAIN_ITERS    4000
#define READER_ITERS    4000
#define WRITER_ITERS    4000

struct SemWorker
{
    volatile ULONG  w_Started;
    volatile ULONG  w_Done;
    volatile ULONG  w_Errors;
    volatile ULONG  w_Progress;
    APTR            w_Arg;
    APTR            w_Arg2;
};

static struct SemWorker *myworker(void)
{
    return (struct SemWorker *)FindTask(NULL)->tc_UserData;
}

static struct Task *spawn(const char *name, APTR pc, int cpu, struct SemWorker *w)
{
#if defined(__AROSEXEC_SMP__)
    if (KernelBase && g_NumCPUs > 1)
    {
        cpumask_t *mask = KrnAllocCPUMask();

        if (mask)
        {
            KrnClearCPUMask(mask);
            KrnGetCPUMask(cpu % g_NumCPUs, mask);
            /* The mask becomes iet_CpuAffinity, freed by exec at teardown. */
            return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                                 TASKTAG_PRI,       0,
                                 TASKTAG_PC,        (IPTR)pc,
                                 TASKTAG_STACKSIZE, STACKSIZE,
                                 TASKTAG_USERDATA,  (IPTR)w,
                                 TASKTAG_AFFINITY,  (IPTR)mask,
                                 TAG_DONE);
        }
    }
#endif
    return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                         TASKTAG_PRI,       0,
                         TASKTAG_PC,        (IPTR)pc,
                         TASKTAG_STACKSIZE, STACKSIZE,
                         TASKTAG_USERDATA,  (IPTR)w,
                         TAG_DONE);
}

static int wait_all(const char *phase, struct SemWorker *w, int n)
{
    int i, t;
    ULONG last = ~0UL;

    for (t = 0; t < WAIT_TICKS; t++)
    {
        int done = 0;
        ULONG sum = 0;

        for (i = 0; i < n; i++)
        {
            if (w[i].w_Done)
                done++;
            sum += w[i].w_Progress;
        }
        if (done == n)
            return 1;
        if ((t % 100) == 99)
        {
            bug("[smpsem] %s: %ds, %d/%d done, progress %lu\n",
                phase, (t + 1) / 50, done, n, (unsigned long)sum);
            if (sum == last)   /* markers included: equal sums = frozen */
            {
                bug("[smpsem] %s: *** STUCK *** (no progress in 2s)\n", phase);
                for (i = 0; i < n; i++)
                    bug("[smpsem]   worker %d: done=%lu progress=%lu\n", i,
                        (unsigned long)w[i].w_Done, (unsigned long)w[i].w_Progress);
                return 0;
            }
            last = sum;
        }
        Delay(1);
    }
    bug("[smpsem] %s: *** TIMEOUT *** (still progressing but too slow)\n", phase);
    return 0;
}

/******************************************************************************/
/*  Phase 1: ObtainSemaphoreList vs single obtainers                          */
/*                                                                            */
/*  One task locks the whole list (its documented contract allows only one   */
/*  such task), while other cores hammer single members. Each member guards  */
/*  its own counter; the list worker checks it owns all members while held.  */
/******************************************************************************/

struct ListCtx
{
    struct List             lc_List;
    struct SignalSemaphore  lc_Sem[LIST_SEMS];
    volatile ULONG          lc_Counter[LIST_SEMS];
};

static void ListWorker(void)
{
    struct SemWorker *w = myworker();
    struct ListCtx *ctx = (struct ListCtx *)w->w_Arg;
    struct Task *me = FindTask(NULL);
    ULONG i;
    int k;

    w->w_Started = 1;

    for (i = 0; i < LIST_ITERS; i++)
    {
        /* Phase markers so a hang dump shows which call we are inside:
         * 1M+i = in ObtainSemaphoreList, 2M+i = critical section,
         * 3M+i = in ReleaseSemaphoreList, i+1 = iteration complete. */
        w->w_Progress = 1000000 + i;
        ObtainSemaphoreList(&ctx->lc_List);
        w->w_Progress = 2000000 + i;
        for (k = 0; k < LIST_SEMS; k++)
        {
            if (ctx->lc_Sem[k].ss_Owner != me)
                w->w_Errors++;
            ctx->lc_Counter[k]++;
        }
        w->w_Progress = 3000000 + i;
        ReleaseSemaphoreList(&ctx->lc_List);
        w->w_Progress = i + 1;
    }

    w->w_Done = 1;
    Wait(0);
}

static void SingleWorker(void)
{
    struct SemWorker *w = myworker();
    struct SignalSemaphore *sem = (struct SignalSemaphore *)w->w_Arg;
    volatile ULONG *counter = (volatile ULONG *)w->w_Arg2;
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < SINGLE_ITERS; i++)
    {
        ULONG v;

        w->w_Progress = 1000000 + i;
        ObtainSemaphore(sem);
        v = *counter;
        *counter = v + 1;
        w->w_Progress = 3000000 + i;
        ReleaseSemaphore(sem);
        w->w_Progress = i + 1;
    }

    w->w_Done = 1;
    Wait(0);
}

static int TestSemList(void)
{
    static struct ListCtx ctx;
    struct SemWorker w[4];
    struct Task *t[4];
    int k, ok = 1;

    bug("[smpsem] phase 1: ObtainSemaphoreList vs single obtainers...\n");

    memset(&ctx, 0, sizeof(ctx));
    memset(w, 0, sizeof(w));
    NEWLIST(&ctx.lc_List);
    for (k = 0; k < LIST_SEMS; k++)
    {
        InitSemaphore(&ctx.lc_Sem[k]);
        AddTail(&ctx.lc_List, (struct Node *)&ctx.lc_Sem[k]);
    }

    /* Uncontended sanity check first: if this hangs, the list path is
     * broken on its own rather than by the contention below. */
    bug("[smpsem] phase 1: uncontended list obtain/release x100...\n");
    for (k = 0; k < 100; k++)
    {
        ObtainSemaphoreList(&ctx.lc_List);
        ReleaseSemaphoreList(&ctx.lc_List);
    }
    bug("[smpsem] phase 1: uncontended OK (nest=%d queue=%d owner=%p)\n",
        (int)ctx.lc_Sem[0].ss_NestCount, (int)ctx.lc_Sem[0].ss_QueueCount,
        ctx.lc_Sem[0].ss_Owner);

    w[0].w_Arg = &ctx;
    t[0] = spawn("smpsem.list", ListWorker, 0, &w[0]);

    /* Three single obtainers on the first three members, one core each. */
    for (k = 1; k < 4; k++)
    {
        w[k].w_Arg  = &ctx.lc_Sem[k - 1];
        w[k].w_Arg2 = (APTR)&ctx.lc_Counter[k - 1];
        t[k] = spawn("smpsem.single", SingleWorker, k, &w[k]);
    }

    for (k = 0; k < 4; k++)
    {
        if (!t[k])
        {
            bug("[smpsem] phase 1: INVALID (task create failed)\n");
            return -1;
        }
    }

    if (!wait_all("phase 1", w, 4))
    {
        /* Dump the frozen state so the deadlock shape is visible. */
        for (k = 0; k < 4; k++)
            bug("[smpsem]   task %d = %p\n", k, t[k]);
        for (k = 0; k < 4; k++)
        {
            ULONG *rf = (ULONG *)t[k]->tc_UnionETask.tc_ETask->et_RegFrame;
            int j;

            bug("[smpsem]   task %d '%s' @%p state=%d sigwait=%08lx sigrecvd=%08lx sigalloc=%08lx\n", k,
                t[k]->tc_Node.ln_Name, t[k],
                (int)t[k]->tc_State,
                (unsigned long)t[k]->tc_SigWait,
                (unsigned long)t[k]->tc_SigRecvd,
                (unsigned long)t[k]->tc_SigAlloc);
            bug("[smpsem]     regframe:");
            for (j = 0; j < 20; j++)
                bug(" %08lx", (unsigned long)rf[j]);
            bug("\n");
            if (k == 0)
            {
                ULONG *sp = (ULONG *)t[k]->tc_SPReg;

                bug("[smpsem]     stack @%p:", sp);
                for (j = 0; j < 48; j++)
                    bug(" %08lx", (unsigned long)sp[j]);
                bug("\n");
            }
        }
        for (k = 0; k < LIST_SEMS; k++)
        {
            struct SignalSemaphore *ss = &ctx.lc_Sem[k];

            bug("[smpsem]   sem %d: owner=%p queue=%d nest=%d waiters=%s mlink-queued=%s\n",
                k, ss->ss_Owner, (int)ss->ss_QueueCount, (int)ss->ss_NestCount,
                (ss->ss_WaitQueue.mlh_Head->mln_Succ != NULL) ? "yes" : "no",
                (ss->ss_MultipleLink.sr_Link.mln_Succ != NULL) ? "yes" : "no");
        }
        ok = 0;
    }

    Forbid();
    for (k = 0; k < 4; k++)
        RemTask(t[k]);
    Permit();

    if (!ok)
        return 0;

    if (w[0].w_Errors)
    {
        bug("[smpsem] phase 1: *** FAIL *** list holder saw %lu foreign owners\n",
            (unsigned long)w[0].w_Errors);
        return 0;
    }
    for (k = 0; k < LIST_SEMS; k++)
    {
        ULONG expect = LIST_ITERS + ((k < 3) ? SINGLE_ITERS : 0);

        if (ctx.lc_Counter[k] != expect)
        {
            bug("[smpsem] phase 1: *** FAIL *** sem %d counter %lu, expected %lu\n",
                k, (unsigned long)ctx.lc_Counter[k], (unsigned long)expect);
            return 0;
        }
    }
    bug("[smpsem] phase 1: OK\n");
    return 1;
}

/******************************************************************************/
/*  Phase 2: Procure/Vacate vs blocking obtainers                             */
/*                                                                            */
/*  Two tasks take the semaphore through the asynchronous bid protocol       */
/*  while two others use plain ObtainSemaphore, all guarding one counter.    */
/******************************************************************************/

struct ProcCtx
{
    struct SignalSemaphore  pc_Sem;
    volatile ULONG          pc_Counter;
};

static void ProcureWorker(void)
{
    struct SemWorker *w = myworker();
    struct ProcCtx *ctx = (struct ProcCtx *)w->w_Arg;
    struct MsgPort *port;
    struct SemaphoreMessage bid;
    ULONG i;

    port = CreateMsgPort();
    if (!port)
    {
        w->w_Errors = 1;
        w->w_Started = 1;
        w->w_Done = 1;
        Wait(0);
    }

    w->w_Started = 1;

    for (i = 0; i < PROCURE_ITERS; i++)
    {
        ULONG v;

        memset(&bid, 0, sizeof(bid));
        bid.ssm_Message.mn_ReplyPort = port;

        Procure(&ctx->pc_Sem, &bid);
        WaitPort(port);
        GetMsg(port);

        if (bid.ssm_Semaphore != &ctx->pc_Sem)
            w->w_Errors++;

        v = ctx->pc_Counter;
        ctx->pc_Counter = v + 1;

        Vacate(&ctx->pc_Sem, &bid);
    }

    DeleteMsgPort(port);
    w->w_Done = 1;
    Wait(0);
}

static void ObtainWorker(void)
{
    struct SemWorker *w = myworker();
    struct ProcCtx *ctx = (struct ProcCtx *)w->w_Arg;
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < OBTAIN_ITERS; i++)
    {
        ULONG v;

        ObtainSemaphore(&ctx->pc_Sem);
        v = ctx->pc_Counter;
        ctx->pc_Counter = v + 1;
        ReleaseSemaphore(&ctx->pc_Sem);
    }

    w->w_Done = 1;
    Wait(0);
}

static int TestProcure(void)
{
    static struct ProcCtx ctx;
    struct SemWorker w[4];
    struct Task *t[4];
    ULONG expect;
    int k, ok = 1;

    bug("[smpsem] phase 2: Procure/Vacate vs ObtainSemaphore...\n");

    memset(&ctx, 0, sizeof(ctx));
    memset(w, 0, sizeof(w));
    InitSemaphore(&ctx.pc_Sem);

    for (k = 0; k < 4; k++)
    {
        w[k].w_Arg = &ctx;
        t[k] = spawn(k < 2 ? "smpsem.procure" : "smpsem.obtain",
                     k < 2 ? (APTR)ProcureWorker : (APTR)ObtainWorker, k, &w[k]);
        if (!t[k])
        {
            bug("[smpsem] phase 2: INVALID (task create failed)\n");
            return -1;
        }
    }

    if (!wait_all("phase 2", w, 4))
        ok = 0;

    Forbid();
    for (k = 0; k < 4; k++)
        RemTask(t[k]);
    Permit();

    if (!ok)
        return 0;

    for (k = 0; k < 4; k++)
    {
        if (w[k].w_Errors)
        {
            bug("[smpsem] phase 2: *** FAIL *** worker %d reported %lu errors\n",
                k, (unsigned long)w[k].w_Errors);
            return 0;
        }
    }
    expect = 2 * PROCURE_ITERS + 2 * OBTAIN_ITERS;
    if (ctx.pc_Counter != expect)
    {
        bug("[smpsem] phase 2: *** FAIL *** counter %lu, expected %lu (lost updates)\n",
            (unsigned long)ctx.pc_Counter, (unsigned long)expect);
        return 0;
    }
    bug("[smpsem] phase 2: OK\n");
    return 1;
}

/******************************************************************************/
/*  Phase 3: shared readers vs exclusive writer                               */
/*                                                                            */
/*  The writer makes the counter odd only while holding the semaphore        */
/*  exclusively; readers holding it shared must never observe an odd value   */
/*  or a value that changes during their hold.                               */
/******************************************************************************/

static void SharedReader(void)
{
    struct SemWorker *w = myworker();
    struct ProcCtx *ctx = (struct ProcCtx *)w->w_Arg;
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < READER_ITERS; i++)
    {
        ULONG a, b;

        ObtainSemaphoreShared(&ctx->pc_Sem);
        a = ctx->pc_Counter;
        b = ctx->pc_Counter;
        ReleaseSemaphore(&ctx->pc_Sem);

        if ((a & 1) || a != b)
            w->w_Errors++;
    }

    w->w_Done = 1;
    Wait(0);
}

static void ExclusiveWriter(void)
{
    struct SemWorker *w = myworker();
    struct ProcCtx *ctx = (struct ProcCtx *)w->w_Arg;
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < WRITER_ITERS; i++)
    {
        ObtainSemaphore(&ctx->pc_Sem);
        ctx->pc_Counter++;      /* odd while held */
        ctx->pc_Counter++;      /* even again before release */
        ReleaseSemaphore(&ctx->pc_Sem);
    }

    w->w_Done = 1;
    Wait(0);
}

static int TestSharedInvariant(void)
{
    static struct ProcCtx ctx;
    struct SemWorker w[4];
    struct Task *t[4];
    int k, ok = 1;

    bug("[smpsem] phase 3: shared readers vs exclusive writer...\n");

    memset(&ctx, 0, sizeof(ctx));
    memset(w, 0, sizeof(w));
    InitSemaphore(&ctx.pc_Sem);

    for (k = 0; k < 4; k++)
    {
        w[k].w_Arg = &ctx;
        t[k] = spawn(k < 3 ? "smpsem.reader" : "smpsem.writer",
                     k < 3 ? (APTR)SharedReader : (APTR)ExclusiveWriter, k, &w[k]);
        if (!t[k])
        {
            bug("[smpsem] phase 3: INVALID (task create failed)\n");
            return -1;
        }
    }

    if (!wait_all("phase 3", w, 4))
        ok = 0;

    Forbid();
    for (k = 0; k < 4; k++)
        RemTask(t[k]);
    Permit();

    if (!ok)
        return 0;

    for (k = 0; k < 3; k++)
    {
        if (w[k].w_Errors)
        {
            bug("[smpsem] phase 3: *** FAIL *** reader %d saw %lu odd/torn values\n",
                k, (unsigned long)w[k].w_Errors);
            return 0;
        }
    }
    if (ctx.pc_Counter != 2 * WRITER_ITERS)
    {
        bug("[smpsem] phase 3: *** FAIL *** counter %lu, expected %lu\n",
            (unsigned long)ctx.pc_Counter, (unsigned long)(2 * WRITER_ITERS));
        return 0;
    }
    bug("[smpsem] phase 3: OK\n");
    return 1;
}

int main(void)
{
    int pass = 0, fail = 0, inval = 0;
    int r;

    KernelBase = OpenResource("kernel.resource");
#if defined(__AROSEXEC_SMP__)
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();
#endif

    bug("[smpsem] start: cpus=%d\n", g_NumCPUs);

    r = TestSemList();          if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestProcure();          if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestSharedInvariant();  if (r > 0) pass++; else if (!r) fail++; else inval++;

    bug("[smpsem] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, inval);
    return fail ? RETURN_FAIL : RETURN_OK;
}
