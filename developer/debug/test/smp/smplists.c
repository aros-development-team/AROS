/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP coverage for the system lists and the library checksum, the
          paths the other SMP tests never touch: AddPort/FindPort/RemPort,
          the semaphore list, the resource list, and SumLibrary racing
          SetFunction.

          The library phase is the sharp one. SumLibrary clears LIBF_CHANGED
          before it sums, and Alert(AT_DeadEnd|AN_LibChkSum)s if the flag was
          already clear and the sum moved. Unless SetFunction and SumLibrary
          exclude each other, a vector patched mid-sum trips that Alert and
          takes the machine down - so a failure here is a dead-end guru, not
          a FAIL line.

          Workers are pinned across cores, so these paths race for real
          instead of being serialised by a single CPU.
*/

#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
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
#define WAIT_TICKS      500         /* 10s bound for Delay-based waits */
#define LIST_WORKERS    4
#define CHURN_ITERS     2000
#define SUM_ITERS       4000
#define SETFUNC_ITERS   4000
#define NAMELEN         32

struct ListWorker
{
    volatile ULONG  w_Started;
    volatile ULONG  w_Done;
    volatile ULONG  w_Errors;
    volatile ULONG  w_Progress;
    int             w_Index;
    APTR            w_Arg;
    char            w_Name[NAMELEN];
};

static struct ListWorker *myworker(void)
{
    return (struct ListWorker *)FindTask(NULL)->tc_UserData;
}

static struct Task *spawn(const char *name, APTR pc, int cpu, struct ListWorker *w)
{
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
    return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                         TASKTAG_PRI,       0,
                         TASKTAG_PC,        (IPTR)pc,
                         TASKTAG_STACKSIZE, STACKSIZE,
                         TASKTAG_USERDATA,  (IPTR)w,
                         TAG_DONE);
}

static int wait_all(const char *phase, struct ListWorker *w, int n)
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
            bug("[smplists] %s: %ds, %d/%d done, progress %lu\n",
                phase, (t + 1) / 50, done, n, (unsigned long)sum);
            if (sum == last)   /* markers included: equal sums = frozen */
            {
                bug("[smplists] %s: *** STUCK *** (no progress in 2s)\n", phase);
                for (i = 0; i < n; i++)
                    bug("[smplists]   worker %d: done=%lu progress=%lu\n", i,
                        (unsigned long)w[i].w_Done, (unsigned long)w[i].w_Progress);
                return 0;
            }
            last = sum;
        }
        Delay(1);
    }
    bug("[smplists] %s: *** TIMEOUT *** (still progressing but too slow)\n", phase);
    return 0;
}

static int collect(const char *phase, struct ListWorker *w, struct Task **t, int n)
{
    int k, ok = wait_all(phase, w, n);

    Forbid();
    for (k = 0; k < n; k++)
        RemTask(t[k]);
    Permit();

    if (!ok)
        return 0;

    for (k = 0; k < n; k++)
    {
        if (w[k].w_Errors)
        {
            bug("[smplists] %s: *** FAIL *** worker %d: %lu errors\n",
                phase, k, (unsigned long)w[k].w_Errors);
            return 0;
        }
    }
    return 1;
}

/******************************************************************************/
/*  Phase 1: the public message port list                                     */
/*                                                                            */
/*  AddPort and FindPort take PortListSpinLock; RemPort has to take the same  */
/*  one. Each worker churns a uniquely named port, so a Find that misses its  */
/*  own entry - or turns one up after removal - means the list was walked     */
/*  while another core was mutating it.                                       */
/******************************************************************************/

static void PortChurn(void)
{
    struct ListWorker *w = myworker();
    struct MsgPort *port = (struct MsgPort *)w->w_Arg;
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < CHURN_ITERS; i++)
    {
        port->mp_Node.ln_Name = w->w_Name;
        AddPort(port);

        if (FindPort(w->w_Name) != port)
            w->w_Errors++;

        RemPort(port);

        if (FindPort(w->w_Name) != NULL)
            w->w_Errors++;

        w->w_Progress = i + 1;
    }

    w->w_Done = 1;
    Wait(0);
}

static int TestPortList(void)
{
    struct ListWorker w[LIST_WORKERS];
    struct Task *t[LIST_WORKERS];
    struct MsgPort port[LIST_WORKERS];
    int k, started = 0;

    bug("[smplists] phase 1: port list churn on %d cores...\n", LIST_WORKERS);

    memset(w, 0, sizeof(w));
    memset(port, 0, sizeof(port));

    for (k = 0; k < LIST_WORKERS; k++)
    {
        NEWLIST(&port[k].mp_MsgList);
        port[k].mp_Flags = PA_IGNORE;
        port[k].mp_Node.ln_Type = NT_MSGPORT;
        snprintf(w[k].w_Name, NAMELEN, "smplists.port.%d", k);
        w[k].w_Index = k;
        w[k].w_Arg = &port[k];

        t[k] = spawn("smplists.port", PortChurn, k, &w[k]);
        if (!t[k])
            break;
        started++;
    }

    if (started != LIST_WORKERS)
    {
        bug("[smplists] phase 1: INVALID (only %d of %d tasks created)\n",
            started, LIST_WORKERS);
        Forbid();
        for (k = 0; k < started; k++)
            RemTask(t[k]);
        Permit();
        return -1;
    }

    if (!collect("phase 1", w, t, LIST_WORKERS))
        return 0;

    bug("[smplists] phase 1: OK\n");
    return 1;
}

/******************************************************************************/
/*  Phase 2: the semaphore list                                               */
/*                                                                            */
/*  RemSemaphore now takes SemListSpinLock, the lock AddSemaphore and         */
/*  FindSemaphore already used.                                               */
/******************************************************************************/

static void SemListChurn(void)
{
    struct ListWorker *w = myworker();
    struct SignalSemaphore *sem = (struct SignalSemaphore *)w->w_Arg;
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < CHURN_ITERS; i++)
    {
        sem->ss_Link.ln_Name = w->w_Name;
        AddSemaphore(sem);

        if (FindSemaphore(w->w_Name) != sem)
            w->w_Errors++;

        RemSemaphore(sem);

        if (FindSemaphore(w->w_Name) != NULL)
            w->w_Errors++;

        w->w_Progress = i + 1;
    }

    w->w_Done = 1;
    Wait(0);
}

static int TestSemList(void)
{
    struct ListWorker w[LIST_WORKERS];
    struct Task *t[LIST_WORKERS];
    struct SignalSemaphore sem[LIST_WORKERS];
    int k, started = 0;

    bug("[smplists] phase 2: semaphore list churn on %d cores...\n", LIST_WORKERS);

    memset(w, 0, sizeof(w));
    memset(sem, 0, sizeof(sem));

    for (k = 0; k < LIST_WORKERS; k++)
    {
        InitSemaphore(&sem[k]);
        snprintf(w[k].w_Name, NAMELEN, "smplists.sem.%d", k);
        w[k].w_Index = k;
        w[k].w_Arg = &sem[k];

        t[k] = spawn("smplists.sem", SemListChurn, k, &w[k]);
        if (!t[k])
            break;
        started++;
    }

    if (started != LIST_WORKERS)
    {
        bug("[smplists] phase 2: INVALID (only %d of %d tasks created)\n",
            started, LIST_WORKERS);
        Forbid();
        for (k = 0; k < started; k++)
            RemTask(t[k]);
        Permit();
        return -1;
    }

    if (!collect("phase 2", w, t, LIST_WORKERS))
        return 0;

    bug("[smplists] phase 2: OK\n");
    return 1;
}

/******************************************************************************/
/*  Phase 3: the resource list                                                */
/*                                                                            */
/*  RemResource moved from bare Forbid() to the list lock OpenResource uses.  */
/******************************************************************************/

static void ResourceChurn(void)
{
    struct ListWorker *w = myworker();
    struct Node *res = (struct Node *)w->w_Arg;
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < CHURN_ITERS; i++)
    {
        res->ln_Name = w->w_Name;
        AddResource(res);

        if (OpenResource(w->w_Name) != (APTR)res)
            w->w_Errors++;

        RemResource(res);

        if (OpenResource(w->w_Name) != NULL)
            w->w_Errors++;

        w->w_Progress = i + 1;
    }

    w->w_Done = 1;
    Wait(0);
}

static int TestResourceList(void)
{
    struct ListWorker w[LIST_WORKERS];
    struct Task *t[LIST_WORKERS];
    struct Node res[LIST_WORKERS];
    int k, started = 0;

    bug("[smplists] phase 3: resource list churn on %d cores...\n", LIST_WORKERS);

    memset(w, 0, sizeof(w));
    memset(res, 0, sizeof(res));

    for (k = 0; k < LIST_WORKERS; k++)
    {
        res[k].ln_Type = NT_RESOURCE;
        snprintf(w[k].w_Name, NAMELEN, "smplists.res.%d", k);
        w[k].w_Index = k;
        w[k].w_Arg = &res[k];

        t[k] = spawn("smplists.res", ResourceChurn, k, &w[k]);
        if (!t[k])
            break;
        started++;
    }

    if (started != LIST_WORKERS)
    {
        bug("[smplists] phase 3: INVALID (only %d of %d tasks created)\n",
            started, LIST_WORKERS);
        Forbid();
        for (k = 0; k < started; k++)
            RemTask(t[k]);
        Permit();
        return -1;
    }

    if (!collect("phase 3", w, t, LIST_WORKERS))
        return 0;

    bug("[smplists] phase 3: OK\n");
    return 1;
}

/******************************************************************************/
/*  Phase 4: SumLibrary against SetFunction                                   */
/*                                                                            */
/*  A private library, never added to the system list, so only this test      */
/*  touches it. Two workers re-sum it while two others patch a vector each.   */
/*  Each patcher owns its own LVO and alternates between two stubs, so the    */
/*  old pointer SetFunction hands back must alternate too - anything else     */
/*  means a concurrent patch landed on our vector. The real verdict, though,  */
/*  is simply surviving: an unserialised sum trips AN_LibChkSum.              */
/******************************************************************************/

static void stubA(void) { }
static void stubB(void) { }
static void stubPad(void) { }

/* Open, Close, Expunge, Reserved, then the vectors the patchers own. */
static CONST_APTR libfuncs[] =
{
    (APTR)stubPad, (APTR)stubPad, (APTR)stubPad, (APTR)stubPad,
    (APTR)stubA,   (APTR)stubA,   (APTR)stubA,   (APTR)stubA,
    (APTR)-1
};

static struct Library *g_Lib;

static void SumChurn(void)
{
    struct ListWorker *w = myworker();
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < SUM_ITERS; i++)
    {
        SumLibrary(g_Lib);
        w->w_Progress = i + 1;
    }

    w->w_Done = 1;
    Wait(0);
}

static void SetFuncChurn(void)
{
    struct ListWorker *w = myworker();
    LONG off = LIB_USERDEF - (LONG)(w->w_Index * LIB_VECTSIZE);
    ULONG i;

    w->w_Started = 1;

    for (i = 0; i < SETFUNC_ITERS; i++)
    {
        APTR old;

        old = SetFunction(g_Lib, off, (APTR)stubB);
        if (old != (APTR)stubA)
            w->w_Errors++;

        old = SetFunction(g_Lib, off, (APTR)stubA);
        if (old != (APTR)stubB)
            w->w_Errors++;

        w->w_Progress = i + 1;
    }

    w->w_Done = 1;
    Wait(0);
}

static int TestLibrarySum(void)
{
    struct ListWorker w[LIST_WORKERS];
    struct Task *t[LIST_WORKERS];
    int k, started = 0;

    bug("[smplists] phase 4: SumLibrary vs SetFunction...\n");

    g_Lib = MakeLibrary((CONST_APTR)libfuncs, NULL, NULL,
                        sizeof(struct Library), (BPTR)0);
    if (!g_Lib)
    {
        bug("[smplists] phase 4: INVALID (MakeLibrary failed)\n");
        return -1;
    }

    g_Lib->lib_Node.ln_Type = NT_LIBRARY;
    g_Lib->lib_Node.ln_Name = "smplists.library";
    g_Lib->lib_Flags |= LIBF_SUMUSED | LIBF_CHANGED;
    SumLibrary(g_Lib);          /* establish the baseline checksum */

    memset(w, 0, sizeof(w));

    /* Two summers, two patchers - the patchers own LVO 0 and 1 past the
     * reserved vectors, so they never collide with each other. */
    for (k = 0; k < LIST_WORKERS; k++)
    {
        w[k].w_Index = (k < 2) ? k : (k - 2);
        t[k] = spawn(k < 2 ? "smplists.sum" : "smplists.setfunc",
                     k < 2 ? (APTR)SumChurn : (APTR)SetFuncChurn, k, &w[k]);
        if (!t[k])
            break;
        started++;
    }

    if (started != LIST_WORKERS)
    {
        bug("[smplists] phase 4: INVALID (only %d of %d tasks created)\n",
            started, LIST_WORKERS);
        Forbid();
        for (k = 0; k < started; k++)
            RemTask(t[k]);
        Permit();
        FreeMem((UBYTE *)g_Lib - g_Lib->lib_NegSize,
                g_Lib->lib_NegSize + g_Lib->lib_PosSize);
        return -1;
    }

    if (!collect("phase 4", w, t, LIST_WORKERS))
        return 0;

    /* Quiesced: one last sum must agree with the jumptable as it stands. */
    g_Lib->lib_Flags |= LIBF_CHANGED;
    SumLibrary(g_Lib);

    FreeMem((UBYTE *)g_Lib - g_Lib->lib_NegSize,
            g_Lib->lib_NegSize + g_Lib->lib_PosSize);
    g_Lib = NULL;

    bug("[smplists] phase 4: OK\n");
    return 1;
}

int main(void)
{
    int pass = 0, fail = 0, inval = 0;
    int r;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        g_NumCPUs = KrnGetCPUCount();

    bug("[smplists] start: cpus=%d\n", g_NumCPUs);

    r = TestPortList();     if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestSemList();      if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestResourceList(); if (r > 0) pass++; else if (!r) fail++; else inval++;
    r = TestLibrarySum();   if (r > 0) pass++; else if (!r) fail++; else inval++;

    bug("[smplists] DONE: %d PASS, %d FAIL, %d INVALID\n", pass, fail, inval);
    return fail ? RETURN_FAIL : RETURN_OK;
}
#else
int main(void)
{
    return RETURN_FAIL;
}
#endif
