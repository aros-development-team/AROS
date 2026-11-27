/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Interrupt servers and IRQ handlers added and removed on every
          CPU while the interrupt they belong to keeps firing.

          On SMP an interrupt chain is walked with a lock held in read
          mode, and adding to or removing from it takes that lock in
          write mode. So:
            - a server or handler does not run any more once the call
              that removes it has returned (its owner may free it),
            - the chain is never seen half updated: the interrupt keeps
              being served, and the servers behind ours keep running,
            - adding and removing do not deadlock.

          Part 1: INTB_VERTB servers, AddIntServer()/RemIntServer().
          One task per CPU adds and removes servers from a pool of its
          own; a permanent server counts the VBlanks meanwhile.

          Part 2: the same with KrnAddIRQHandler()/KrnRemIRQHandler() on
          the interrupt timer.device runs from, where the port has one
          a program can find out.

          Part 3 (pc): KrnAllocIRQ(IRQTYPE_APIC) from several CPUs at
          once must not hand out one interrupt twice, and removing the
          last handler of such an interrupt must return. Where it does
          not, that CPU is lost with its interrupts off: this part is
          the last one, and this test is best run late.

          A record is marked dead as soon as its removal has returned,
          and is used again only after the rest of its pool has been.
          The servers do nothing but look at their own record, so the
          test needs no Disable() or Forbid() anywhere.
*/

#include <aros/config.h>
#include <aros/asmcall.h>
#include <aros/kernel.h>

#include <exec/interrupts.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <resources/task.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>
#include <proto/task.h>

#include <string.h>

#if defined(__AROSEXEC_SMP__)
#define DEBUG 1
#include <aros/debug.h>

#if defined(__riscv) && defined(KATTR_PlatformTimer) && defined(__has_include)
#if __has_include(<aros/platformtimer.h>)
#include <aros/platformtimer.h>
#define HAVE_PLATFORMTIMER 1
#endif
#endif

APTR KernelBase;
APTR TaskResBase;

#define STACKSIZE   16384
#define WAIT_TICKS  250             /* 5s */
#define MAXTASKS    16
#define POOL        32              /* records per task */

#define VERTB_SECS  8               /* part 1 churn time */
#define KIRQ_SECS   6               /* part 2 churn time */
#define STALL_SECS  2               /* VBlank may stand still this long */

#define MAGIC_LIVE  0x4C495645UL
#define MAGIC_DEAD  0xDEADDEADUL

#define MODE_VERTB  0
#define MODE_KIRQ   1
#define MODES       2

#if defined(__x86_64__)
#ifndef IRQTYPE_APIC
#define IRQTYPE_APIC    (1 << 3)    /* aros/irqtypes.h */
#endif
#define MSI_TASKS   4
#define MSI_ALLOCS  3
#define MSI_COUNT   2
#endif

struct ChurnRec
{
    struct Interrupt    cr_Int;     /* MODE_VERTB */
    APTR                cr_Handle;  /* MODE_KIRQ */
    volatile ULONG      cr_Magic;
    volatile ULONG      cr_Runs;
    volatile ULONG      cr_In;      /* the server is inside */
};

struct ChurnTask
{
    struct ChurnRec    *ct_Pool;
    struct Task        *ct_Task;
    ULONG               ct_CPU;
    ULONG               ct_Mode;
    ULONG               ct_IRQ;
    volatile ULONG      ct_Stop;
    volatile ULONG      ct_Started;
    volatile ULONG      ct_Done;
    volatile ULONG      ct_Iters;
    volatile ULONG      ct_AddFail; /* KrnAddIRQHandler() returned NULL */
    volatile ULONG      ct_StillIn; /* removal returned with the server inside */
    volatile ULONG      ct_Spin;
};

/* Never freed and never reused by another part: a task that hangs keeps its part's */
static struct ChurnRec  churnrec[MODES][MAXTASKS][POOL];
static struct ChurnTask churntask[MODES][MAXTASKS];

static struct Interrupt tickserver;
static volatile ULONG   g_Ticks;        /* VBlanks seen by the permanent server */
static volatile ULONG   g_KTicks;       /* part 2: interrupts seen by the permanent handler */
static volatile ULONG   g_AfterRem;     /* runs of a record that was not live */

static int pass, fail, inval;

static void ok(int part, const char *what)
{
    bug("[smpirqchurn] part %d: OK - %s\n", (int)part, what);
    pass++;
}

/*
 * Runs in interrupt context, on whichever CPU takes the interrupt.
 * Touches nothing but its own record and one counter.
 */
static inline void churn_hit(struct ChurnRec *r)
{
    __atomic_store_n(&r->cr_In, 1, __ATOMIC_SEQ_CST);
    if (__atomic_load_n(&r->cr_Magic, __ATOMIC_SEQ_CST) != MAGIC_LIVE)
        __atomic_fetch_add(&g_AfterRem, 1, __ATOMIC_SEQ_CST);
    else
        __atomic_fetch_add(&r->cr_Runs, 1, __ATOMIC_SEQ_CST);
    __atomic_store_n(&r->cr_In, 0, __ATOMIC_SEQ_CST);
}

static AROS_INTH1(ChurnServer, struct ChurnRec *, r)
{
    AROS_INTFUNC_INIT

    churn_hit(r);

    /* Not ours: exec goes on with the servers behind this one */
    return 0;

    AROS_INTFUNC_EXIT
}

static void ChurnHandler(void *data1, void *data2)
{
    churn_hit((struct ChurnRec *)data1);
}

static AROS_INTH1(TickServer, volatile ULONG *, count)
{
    AROS_INTFUNC_INIT

    __atomic_fetch_add(count, 1, __ATOMIC_SEQ_CST);

    return 0;

    AROS_INTFUNC_EXIT
}

static void CountHandler(void *data1, void *data2)
{
    __atomic_fetch_add((volatile ULONG *)data1, 1, __ATOMIC_SEQ_CST);
}

static inline ULONG rd(volatile ULONG *p)
{
    return __atomic_load_n(p, __ATOMIC_SEQ_CST);
}

static void ChurnWorker(void)
{
    struct ChurnTask *t = (struct ChurnTask *)FindTask(NULL)->tc_UserData;
    struct ChurnRec *r;
    ULONG i = 0, j, sel;

    t->ct_Started = 1;

    while (!t->ct_Stop)
    {
        r = &t->ct_Pool[i % POOL];
        sel = (i + t->ct_CPU) & 15;
        i++;

        __atomic_store_n(&r->cr_Magic, MAGIC_LIVE, __ATOMIC_SEQ_CST);
        if (t->ct_Mode == MODE_VERTB)
            AddIntServer(INTB_VERTB, &r->cr_Int);
        else
        {
            r->cr_Handle = KrnAddIRQHandler(t->ct_IRQ, (APTR)ChurnHandler, r, NULL);
            if (r->cr_Handle == NULL)
            {
                __atomic_store_n(&r->cr_Magic, MAGIC_DEAD, __ATOMIC_SEQ_CST);
                t->ct_AddFail++;
                continue;
            }
        }

        /* Out again at once, after a moment, or after a VBlank or so */
        if (sel == 0)
            Delay(1);
        else if (sel > 4)
        {
            for (j = 0; j < sel * 200; j++)
                t->ct_Spin++;
        }

        if (t->ct_Mode == MODE_VERTB)
            RemIntServer(INTB_VERTB, &r->cr_Int);
        else
            KrnRemIRQHandler(r->cr_Handle);
        __atomic_store_n(&r->cr_Magic, MAGIC_DEAD, __ATOMIC_SEQ_CST);

        if (rd(&r->cr_In))
            t->ct_StillIn++;

        t->ct_Iters = i;
    }

    t->ct_Done = 1;
}

static struct Task *spawn(const char *name, APTR pc, int cpu, APTR data)
{
    cpumask_t *mask = KrnAllocCPUMask();

    if (mask == NULL)
        return NULL;
    KrnClearCPUMask(mask);
    KrnGetCPUMask(cpu, mask);

    return NewCreateTask(TASKTAG_NAME,      (IPTR)name,
                         TASKTAG_PRI,       0,
                         TASKTAG_PC,        (IPTR)pc,
                         TASKTAG_STACKSIZE, STACKSIZE,
                         TASKTAG_USERDATA,  (IPTR)data,
                         TASKTAG_AFFINITY,  (IPTR)mask,
                         TAG_DONE);
}

static int wait_flag(volatile ULONG *flag, int ticks)
{
    int i;

    for (i = 0; i < ticks && !*flag; i++)
        Delay(1);
    return *flag != 0;
}

/*
 * One churn task per CPU for 'seconds'. Returns 0 when every task came
 * to its end, 1 when one did not (nothing more can be tested then) and
 * -1 when the part could not be run.
 */
static int run_churn(int part, int mode, ULONG irq, int ntasks, int seconds)
{
    struct ChurnTask *t;
    struct ChurnRec *r;
    ULONG last, now, iters = 0, runs = 0, addfail = 0, stillin = 0, after;
    int i, k, s, stall = 0, stopped = 0, created = 0, hung = 0, left;

    __atomic_store_n(&g_AfterRem, 0, __ATOMIC_SEQ_CST);

    for (i = 0; i < ntasks; i++)
    {
        t = &churntask[mode][i];
        memset(t, 0, sizeof(*t));
        t->ct_Pool = churnrec[mode][i];
        t->ct_CPU  = i;
        t->ct_Mode = mode;
        t->ct_IRQ  = irq;

        for (k = 0; k < POOL; k++)
        {
            r = &t->ct_Pool[k];
            memset(r, 0, sizeof(*r));
            r->cr_Int.is_Node.ln_Type = NT_INTERRUPT;
            r->cr_Int.is_Node.ln_Name = "smpirqchurn";
            /* Before, among and behind the system's own servers */
            r->cr_Int.is_Node.ln_Pri  = (BYTE)(((i % 8) * 4) - 16);
            r->cr_Int.is_Code         = (VOID_FUNC)ChurnServer;
            r->cr_Int.is_Data         = r;
            r->cr_Magic               = MAGIC_DEAD;
        }
    }

    for (i = 0; i < ntasks; i++)
    {
        t = &churntask[mode][i];
        t->ct_Task = spawn("smpirqchurn.churn", ChurnWorker, i, t);
        if (t->ct_Task == NULL)
        {
            bug("[smpirqchurn] part %d: no churn task for CPU %d\n", (int)part, (int)i);
            continue;
        }
        created++;
        if (!wait_flag(&t->ct_Started, WAIT_TICKS))
            bug("[smpirqchurn] part %d: churn task on CPU %d did not start\n", (int)part, (int)i);
    }

    if (!created)
    {
        bug("[smpirqchurn] part %d: INVALID - no churn task could be created\n", (int)part);
        inval++;
        return -1;
    }

    bug("[smpirqchurn] part %d: %d tasks churning for %d seconds\n",
        (int)part, (int)created, (int)seconds);

    last = rd(&g_Ticks);
    for (s = 0; s < seconds; s++)
    {
        Delay(50);
        now = rd(&g_Ticks);
        if (now != last)
            stall = 0;
        else if (++stall > STALL_SECS)
        {
            stopped = 1;
            break;
        }
        last = now;
    }

    for (i = 0; i < ntasks; i++)
        churntask[mode][i].ct_Stop = 1;

    /* One time limit for all of them */
    for (s = 0; s < WAIT_TICKS; s++)
    {
        left = 0;
        for (i = 0; i < ntasks; i++)
        {
            t = &churntask[mode][i];
            if (t->ct_Task && !t->ct_Done)
                left++;
        }
        if (!left)
            break;
        Delay(1);
    }

    for (i = 0; i < ntasks; i++)
    {
        t = &churntask[mode][i];
        if (t->ct_Task == NULL)
            continue;
        if (!t->ct_Done)
        {
            bug("[smpirqchurn] part %d: *** FAIL *** churn task on CPU %d did not finish "
                "(%lu iterations)\n", (int)part, (int)i, (unsigned long)t->ct_Iters);
            fail++;
            hung = 1;
        }
        iters   += t->ct_Iters;
        addfail += t->ct_AddFail;
        stillin += t->ct_StillIn;
        for (k = 0; k < POOL; k++)
            runs += rd(&t->ct_Pool[k].cr_Runs);
    }
    after = rd(&g_AfterRem);

    bug("[smpirqchurn] part %d: %lu iterations, %lu server runs, %lu failed adds\n",
        (int)part, (unsigned long)iters, (unsigned long)runs, (unsigned long)addfail);

    if (!hung)
        ok(part, "every churn task finished");

    if (stopped)
    {
        bug("[smpirqchurn] part %d: *** FAIL *** VBlank stopped: the permanent server "
            "did not run for %d seconds\n", (int)part, (int)(STALL_SECS + 1));
        fail++;
    }
    else
        ok(part, "VBlank kept running during the churn");

    if (after)
    {
        bug("[smpirqchurn] part %d: *** FAIL *** server ran after %s returned (%lu times)\n",
            (int)part, mode == MODE_VERTB ? "RemIntServer()" : "KrnRemIRQHandler()",
            (unsigned long)after);
        fail++;
    }
    else
        ok(part, "no server ran after its removal");

    if (stillin)
    {
        bug("[smpirqchurn] part %d: *** FAIL *** %s returned while the server was "
            "still running (%lu times)\n",
            (int)part, mode == MODE_VERTB ? "RemIntServer()" : "KrnRemIRQHandler()",
            (unsigned long)stillin);
        fail++;
    }
    else
        ok(part, "no removal returned with its server running");

    if (runs == 0)
    {
        bug("[smpirqchurn] part %d: INVALID - none of the churned servers ever ran\n", (int)part);
        inval++;
    }

    if (hung)
        return 1;

    /* With everything of ours off the chain again it must still be served */
    last = rd(&g_Ticks);
    Delay(50);
    if (rd(&g_Ticks) == last)
    {
        bug("[smpirqchurn] part %d: *** FAIL *** VBlank stopped after the churn\n", (int)part);
        fail++;
    }
    else
        ok(part, "VBlank runs after the churn");

    return 0;
}

/*
 * The interrupt timer.device is driven by, where a program can know it.
 * It fires at least at the VBlank rate, and its chain always holds
 * timer.device's handler, so our removals never empty it. -1 for none.
 */
static LONG live_irq(void)
{
#if defined(__x86_64__)
    /* The PIT: arch/all-pc/timer/timer_init.c, KrnAddIRQHandler(0, TimerInt, ...) */
    return 0;
#elif defined(HAVE_PLATFORMTIMER)
    /* A virtual source the kernel names: arch/riscv64-sbi/timer/timer_init.c */
    struct KrnPlatformTimer *kpt;

    kpt = (struct KrnPlatformTimer *)KrnGetSystemAttr(KATTR_PlatformTimer);
    if (!kpt || kpt == (APTR)-1 || kpt->kpt_IRQ == (ULONG)-1)
        return -1;
    return (LONG)kpt->kpt_IRQ;
#elif defined(__aarch64__)
    /*
     * arch/aarch64-raspi/timer/timer_init.c: system timer compare 1,
     * IRQ_TIMER0 + TICK_TIMER = 1. On the BCM2711 and BCM2712 it sits
     * behind the GIC and is not delivered at all, so only the others.
     */
    IPTR base = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);

    if (base == 0 || base == (IPTR)-1 ||
        base == (IPTR)0xFE000000UL || base == (IPTR)0x107C000000ULL)
        return -1;
    return 1;
#else
    return -1;
#endif
}

/* Returns 1 when a churn task hung */
static int part2(int ntasks)
{
    LONG irq = live_irq();
    APTR perm;
    ULONG before, during;
    int r;

    if (irq < 0)
    {
        bug("[smpirqchurn] part 2: SKIP (no known live IRQ on this port)\n");
        return 0;
    }

    __atomic_store_n(&g_KTicks, 0, __ATOMIC_SEQ_CST);
    perm = KrnAddIRQHandler((ULONG)irq, (APTR)CountHandler, (APTR)&g_KTicks, NULL);
    if (perm == NULL)
    {
        bug("[smpirqchurn] part 2: INVALID - KrnAddIRQHandler(%d) failed\n", irq);
        inval++;
        return 0;
    }

    /* Is it really firing? */
    Delay(50);
    before = rd(&g_KTicks);
    bug("[smpirqchurn] part 2: IRQ %d fired %lu times in a second\n", irq, (unsigned long)before);
    if (before < 5)
    {
        KrnRemIRQHandler(perm);
#if defined(__aarch64__)
        bug("[smpirqchurn] part 2: SKIP (IRQ %d does not fire on this machine)\n", irq);
#else
        bug("[smpirqchurn] part 2: INVALID - IRQ %d does not fire\n", irq);
        inval++;
#endif
        return 0;
    }

    r = run_churn(2, MODE_KIRQ, (ULONG)irq, ntasks, KIRQ_SECS);
    during = rd(&g_KTicks);

    if (r < 0)
    {
        KrnRemIRQHandler(perm);
        return 0;
    }

    if (during == before)
    {
        bug("[smpirqchurn] part 2: *** FAIL *** IRQ %d was not served during the churn\n", irq);
        fail++;
    }
    else
        ok(2, "the interrupt kept being served");

    /* After a hang the write lock may never come: leave the handler in */
    if (r > 0)
        return 1;

    KrnRemIRQHandler(perm);
    before = rd(&g_Ticks);
    Delay(25);
    if (rd(&g_Ticks) == before)
    {
        bug("[smpirqchurn] part 2: *** FAIL *** VBlank stopped after the last "
            "KrnRemIRQHandler()\n");
        fail++;
    }

    return 0;
}

#if defined(__x86_64__)
struct MsiTask
{
    struct Task        *mt_Task;
    volatile ULONG      mt_Ready;
    volatile ULONG      mt_Done;
    volatile ULONG      mt_IRQ[MSI_ALLOCS];
    /* the remove test */
    volatile ULONG      mt_NoHandle;
    volatile ULONG      mt_Added;
};

static struct MsiTask   msitask[MSI_TASKS], msirem;
static volatile ULONG   g_Go;
static volatile ULONG   g_MsiHits;

static void MsiAllocWorker(void)
{
    struct MsiTask *m = (struct MsiTask *)FindTask(NULL)->tc_UserData;
    int k;

    m->mt_Ready = 1;
    while (!g_Go)
        ;

    for (k = 0; k < MSI_ALLOCS; k++)
        m->mt_IRQ[k] = KrnAllocIRQ(IRQTYPE_APIC, MSI_COUNT);

    m->mt_Done = 1;
}

static void MsiRemWorker(void)
{
    struct MsiTask *m = (struct MsiTask *)FindTask(NULL)->tc_UserData;
    APTR handle;
    ULONG irq;

    irq = KrnAllocIRQ(IRQTYPE_APIC, 1);
    m->mt_IRQ[0] = irq;
    m->mt_Ready = 1;
    if (irq != (ULONG)-1)
    {
        handle = KrnAddIRQHandler(irq, (APTR)CountHandler, (APTR)&g_MsiHits, NULL);
        if (handle == NULL)
            m->mt_NoHandle = 1;
        else
        {
            m->mt_Added = 1;
            /* The only handler of the interrupt: this disables it */
            KrnRemIRQHandler(handle);
        }
    }

    m->mt_Done = 1;
}

static void part3(int ncpus)
{
    struct MsiTask *a, *b;
    cpumask_t *mask;
    ULONG ia, ib;
    int n, i, j, k, l, created = 0, got = 0, overlap = 0, late = 0, pinned = 0;

    /* (b) KrnAllocIRQ() on several CPUs at the same moment */
    n = (ncpus < MSI_TASKS) ? ncpus : MSI_TASKS;
    memset(msitask, 0, sizeof(msitask));
    g_Go = 0;

    for (i = 0; i < n; i++)
    {
        a = &msitask[i];
        for (k = 0; k < MSI_ALLOCS; k++)
            a->mt_IRQ[k] = (ULONG)-1;
        a->mt_Task = spawn("smpirqchurn.msi", MsiAllocWorker, i, a);
        if (a->mt_Task)
            created++;
    }
    for (i = 0; i < n; i++)
    {
        if (msitask[i].mt_Task)
            wait_flag(&msitask[i].mt_Ready, WAIT_TICKS);
    }
    g_Go = 1;
    for (i = 0; i < n; i++)
    {
        if (msitask[i].mt_Task && !wait_flag(&msitask[i].mt_Done, WAIT_TICKS))
        {
            bug("[smpirqchurn] part 3: *** FAIL *** KrnAllocIRQ() did not return on CPU %d\n",
                (int)i);
            fail++;
            late = 1;
        }
    }
    if (late)
        return;

    if (created < 2)
    {
        bug("[smpirqchurn] part 3: INVALID - no two allocating tasks\n");
        inval++;
        return;
    }

    for (i = 0; i < n; i++)
    {
        a = &msitask[i];
        for (k = 0; k < MSI_ALLOCS; k++)
        {
            ia = a->mt_IRQ[k];
            if (ia == (ULONG)-1)
                continue;
            got++;
            bug("[smpirqchurn] part 3: CPU %d got interrupts %lu-%lu\n",
                (int)i, (unsigned long)ia, (unsigned long)(ia + MSI_COUNT - 1));

            /* against every later one, this task's own included */
            for (j = i; j < n; j++)
            {
                b = &msitask[j];
                for (l = (j == i) ? k + 1 : 0; l < MSI_ALLOCS; l++)
                {
                    ib = b->mt_IRQ[l];
                    if (ib == (ULONG)-1)
                        continue;
                    if (ia < ib + MSI_COUNT && ib < ia + MSI_COUNT)
                        overlap++;
                }
            }
        }
    }

    if (!got)
    {
        bug("[smpirqchurn] part 3: SKIP (KrnAllocIRQ(IRQTYPE_APIC) gives nothing: no MSI here)\n");
        return;
    }

    if (overlap)
    {
        bug("[smpirqchurn] part 3: *** FAIL *** KrnAllocIRQ() gave the same interrupt "
            "to two callers (%d overlapping ranges)\n", (int)overlap);
        fail++;
    }
    else
        ok(3, "concurrent KrnAllocIRQ() ranges do not overlap");

    /*
     * (a) Remove the last handler of an APIC interrupt, on the last CPU.
     * If that hangs the CPU is gone: stay away from it ourselves.
     */
    if (TaskResBase && (mask = KrnAllocCPUMask()) != NULL)
    {
        KrnClearCPUMask(mask);
        KrnGetCPUMask(0, mask);
        pinned = SetTaskAffinity(NULL, mask) ? 1 : 0;
        KrnFreeCPUMask(mask);
    }

    memset(&msirem, 0, sizeof(msirem));
    msirem.mt_IRQ[0] = (ULONG)-1;
    msirem.mt_Task = spawn("smpirqchurn.msirem", MsiRemWorker, ncpus - 1, &msirem);
    if (msirem.mt_Task == NULL || !wait_flag(&msirem.mt_Ready, WAIT_TICKS))
    {
        bug("[smpirqchurn] part 3: INVALID - the removing task did not start\n");
        inval++;
    }
    else if (!wait_flag(&msirem.mt_Done, WAIT_TICKS))
    {
        if (msirem.mt_Added)
            bug("[smpirqchurn] part 3: *** FAIL *** KrnRemIRQHandler() of the last handler "
                "of an APIC interrupt did not return (interrupt %lu, CPU %d)\n",
                (unsigned long)msirem.mt_IRQ[0], (int)(ncpus - 1));
        else
            bug("[smpirqchurn] part 3: *** FAIL *** KrnAddIRQHandler() for an APIC interrupt "
                "did not return (interrupt %lu, CPU %d)\n",
                (unsigned long)msirem.mt_IRQ[0], (int)(ncpus - 1));
        fail++;
        /* Stay on CPU 0 */
        return;
    }
    else if (msirem.mt_IRQ[0] == (ULONG)-1)
        bug("[smpirqchurn] part 3: SKIP remove (no interrupt left to allocate)\n");
    else if (msirem.mt_NoHandle)
    {
        bug("[smpirqchurn] part 3: INVALID - KrnAddIRQHandler(%lu) failed\n",
            (unsigned long)msirem.mt_IRQ[0]);
        inval++;
    }
    else
    {
        bug("[smpirqchurn] part 3: interrupt %lu: handler added and removed on CPU %d\n",
            (unsigned long)msirem.mt_IRQ[0], (int)(ncpus - 1));
        ok(3, "KrnRemIRQHandler() of the last handler of an APIC interrupt returned");
    }

    if (pinned)
        SetTaskAffinity(NULL, (APTR)TASKAFFINITY_ANY);
}
#endif /* __x86_64__ */

int main(void)
{
    int ncpus, ntasks, hung;
    ULONG t0;

    KernelBase = OpenResource("kernel.resource");
    TaskResBase = OpenResource("task.resource");
    ncpus = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smpirqchurn] start: cpus=%d\n", (int)ncpus);
    if (ncpus < 2)
    {
        bug("[smpirqchurn] only %d CPU online - needs SMP; SKIP\n", (int)ncpus);
        return RETURN_WARN;
    }
    ntasks = (ncpus < MAXTASKS) ? ncpus : MAXTASKS;

    tickserver.is_Node.ln_Type = NT_INTERRUPT;
    tickserver.is_Node.ln_Name = "smpirqchurn.tick";
    tickserver.is_Node.ln_Pri  = -32;       /* behind all the churned ones */
    tickserver.is_Code         = (VOID_FUNC)TickServer;
    tickserver.is_Data         = (APTR)&g_Ticks;
    AddIntServer(INTB_VERTB, &tickserver);

    t0 = rd(&g_Ticks);
    Delay(25);
    if (rd(&g_Ticks) == t0)
    {
        RemIntServer(INTB_VERTB, &tickserver);
        bug("[smpirqchurn] INVALID: the VBlank server never ran\n");
        bug("[smpirqchurn] DONE: 0 PASS, 0 FAIL, 1 INVALID\n");
        return RETURN_WARN;
    }

    hung = run_churn(1, MODE_VERTB, 0, ntasks, VERTB_SECS) > 0;

    if (!hung)
        hung = part2(ntasks);
    else
        bug("[smpirqchurn] part 2: not run after a hang\n");

#if defined(__x86_64__)
    if (!hung)
        part3(ncpus);
    else
        bug("[smpirqchurn] part 3: not run after a hang\n");
#endif

    /* First the verdict: after a hang the removal below may not come back */
    bug("[smpirqchurn] DONE: %d PASS, %d FAIL, %d INVALID\n",
        (int)pass, (int)fail, (int)inval);

    RemIntServer(INTB_VERTB, &tickserver);

    return fail ? RETURN_FAIL : (inval ? RETURN_WARN : RETURN_OK);
}
#else
int main(void)
{
    return RETURN_WARN;
}
#endif
