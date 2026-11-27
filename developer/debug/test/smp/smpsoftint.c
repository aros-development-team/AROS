/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Software interrupts on SMP.

          On SMP a software interrupt is served by the boot CPU alone,
          whichever CPU calls Cause(). So:
            - a handler runs on CPU 0 and in supervisor mode,
            - no two handlers run at the same time, and none is entered
              again while it runs (as with a single CPU),
            - every Cause() is served: one run for an interrupt that is
              neither queued nor running, none extra while it is queued,
              one more after the current one while it is running,
            - a Cause() made on another CPU is served at once (the boot
              CPU is told), not by its next timer tick,
            - an interrupt handler calling Signal() while the system
              switches tasks does not deadlock a CPU on the run list.

          Part 1: one soft interrupt per priority, Cause()d by a task on
          every CPU. The handlers check where and how they run.

          Part 2: one soft interrupt, Cause()d by a task on every CPU,
          first one CPU at a time (every Cause() must lead to exactly one
          run), then all together. Each task waits for a run that started
          after its Cause(); one that does not come is a lost Cause().

          Part 3: the time from a Cause() on the last CPU to the run,
          taken right after a VBlank, 50 times. With the boot CPU told
          the run comes long before the next VBlank.

          Part 4: stress. A VBlank server, and a handler on the timer's
          interrupt where the port has one a program can find out, call
          Signal() and Cause(); the soft interrupts call Signal(); two
          tasks on every CPU wake each other as fast as they can. All of
          it has to keep going for ten seconds. Where it hangs every CPU
          spins with its interrupts off and nothing more is printed.

          The handlers touch their own record and atomic counters only,
          and tasks outlive everything that signals them, so the test
          needs no Disable() or Forbid() anywhere.
*/

#include <aros/config.h>
#include <aros/asmcall.h>
#include <aros/kernel.h>

#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>
#include <utility/tagitem.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>

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

#define STACKSIZE       16384
#define WAIT_TICKS      250             /* 5s */
#define MAXTASKS        16
#define NPRI            5

#define P1_TICKS        250             /* part 1: 5s of Cause() */
#define P1_SPIN         2000            /* handler stays inside this long */

#define P2_SOLO_TICKS   10              /* part 2: each CPU alone */
#define P2_ALL_TICKS    100             /* part 2: all CPUs together */
#define TIMEOUT_TICKS   100             /* a Cause() not served in 2s is lost */

/*
 * Part 3. Each Cause() is made right after a VBlank, so a run that only
 * comes with the boot CPU's next tick is seen behind the next VBlank,
 * nearly a full period later. A wait is "slow" when the VBlank counter
 * moved during it and, by the calibrated loop, it took at least half a
 * period (the second condition drops waits that started late because
 * this task was held up between the VBlank and its Cause()).
 *
 *   more than P3_SLOW_MAX of the P3_ROUNDS waits slow   -> FAIL
 *   the same, but the calibration is not to be trusted  -> INVALID
 *   a wait of TIMEOUT_TICKS                             -> FAIL (lost)
 *
 * The calibration (loop iterations per VBlank, over P3_CAL_TICKS) is
 * taken before and after the waits. It is not trusted when one of the
 * two is below P3_CAL_MIN or they differ by more than P3_CAL_RATIO: the
 * host was too busy for the loop to measure anything. A few slow waits
 * are a virtual CPU that was not run for a while, and pass. Waits of
 * P3_VSLOW_TICKS and more are counted and printed only.
 */
#define P3_ROUNDS       50
#define P3_SLOW_MAX     5
#define P3_VSLOW_TICKS  3
#define P3_CAL_TICKS    8
#define P3_CAL_MIN      64
#define P3_CAL_RATIO    4
#define LOOP_CAP        0x20000000UL    /* ends a wait when the VBlank is gone too */

#define P4_SECS         10
#define P4_STALL        3               /* samples without progress */
#define P4_EXTRA        4               /* counters besides the CPUs' */

#define IDLE_TICKS      3

#define TURN_NONE       (-2)
#define TURN_ALL        (-1)

static const BYTE softpri[NPRI] = { -32, -16, 0, 16, 32 };

struct SoftRec
{
    struct Interrupt        sr_Int;
    ULONG                   sr_Spin;
    struct Task * volatile  sr_SigTask;     /* part 4: woken by every run */
    volatile ULONG          sr_SigMask;
    volatile ULONG          sr_Runs;
    volatile ULONG          sr_Started;     /* number of the last run begun */
    volatile ULONG          sr_Completed;   /* number of the last run ended */
    volatile ULONG          sr_In;
    volatile ULONG          sr_WrongCPU;
    volatile ULONG          sr_LastCPU;
    volatile ULONG          sr_Overlap;
    volatile ULONG          sr_Reentered;
    volatile ULONG          sr_NotSuper;
    volatile ULONG          sr_Sink;
};

/* Parts 1 to 3 */
struct Worker
{
    struct Task            *w_Task;
    ULONG                   w_CPU;
    volatile ULONG          w_Stop;
    volatile ULONG          w_Started;
    volatile ULONG          w_Done;
    volatile ULONG          w_Causes;
    volatile ULONG          w_Spin;
    volatile LONG           w_Seen;         /* part 2: the turn it last read */
    volatile ULONG          w_GaveUp;
    volatile ULONG          w_Timeouts;
    volatile ULONG          w_WorstTicks;
    volatile ULONG          w_WorstIter;
};

/* Part 4: the two waiters and the tasks waking each other */
struct SigTask
{
    struct Task            *st_Created;
    struct Task * volatile  st_Task;
    struct SigTask         *st_Peer;
    volatile ULONG          st_Mask;
    volatile ULONG          st_Ready;
    volatile ULONG          st_NoSig;
    volatile ULONG          st_Quiet;       /* signals nobody any more */
    volatile ULONG          st_Done;
    volatile ULONG          st_Count;
};

/* Part 4: what an interrupt does each time */
struct HwSrc
{
    struct Interrupt        hs_Int;
    struct Task            *hs_Task;
    ULONG                   hs_Mask;
    struct SoftRec         *hs_Soft;
    volatile ULONG          hs_Hits;
};

/* Never freed and never used by another part: a task that hangs keeps its part's */
static struct SoftRec   p1soft[NPRI], sint, p4soft[2];
static struct Worker    p1work[MAXTASKS], p2work[MAXTASKS], p3work;
static struct SigTask   pptask[MAXTASKS][2], waita, waitb;
static struct HwSrc     hwvert, hwkirq;

static struct Interrupt tickserver;
static volatile ULONG   g_Ticks;        /* VBlanks seen by the permanent server */
static volatile ULONG   g_Inside;       /* soft interrupt handlers running now */
static volatile ULONG   g_Never;        /* stays 0 */
static volatile LONG    g_P2Turn;
static volatile ULONG   g_P4Go, g_P4Stop, g_P4End;

static ULONG            p3iter[P3_ROUNDS], p3tick[P3_ROUNDS];
static volatile ULONG   p3done, p3timeout, p3nosync, p3cal1, p3cal2;

static int pass, fail, inval, verdict_given;

static void ok(int part, const char *what)
{
    bug("[smpsoftint] part %d: OK - %s\n", (int)part, what);
    pass++;
}

/* First the verdict where what follows may not come back */
static void verdict(void)
{
    if (!verdict_given)
        bug("[smpsoftint] DONE: %d PASS, %d FAIL, %d INVALID\n",
            (int)pass, (int)fail, (int)inval);
    verdict_given = 1;
}

static inline ULONG rd(volatile ULONG *p)
{
    return __atomic_load_n(p, __ATOMIC_SEQ_CST);
}

static inline void count(volatile ULONG *p)
{
    __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST);
}

/*
 * Every soft interrupt of this test. Hardware interrupts may come in
 * between, tasks and other soft interrupts may not: g_Inside is 0 on
 * entry, or a second handler is running somewhere.
 */
static AROS_INTH1(SoftHandler, struct SoftRec *, r)
{
    AROS_INTFUNC_INIT

    struct Task *task;
    ULONG cpu, seq, j;

    cpu = KrnGetCPUNumber();
    if (cpu != 0)
    {
        __atomic_store_n(&r->sr_LastCPU, cpu, __ATOMIC_SEQ_CST);
        count(&r->sr_WrongCPU);
    }
    if (__atomic_fetch_add(&g_Inside, 1, __ATOMIC_SEQ_CST) != 0)
        count(&r->sr_Overlap);
    if (__atomic_exchange_n(&r->sr_In, 1, __ATOMIC_SEQ_CST) != 0)
        count(&r->sr_Reentered);
    if (!KrnIsSuper())
        count(&r->sr_NotSuper);

    seq = __atomic_add_fetch(&r->sr_Started, 1, __ATOMIC_SEQ_CST);

    /* Long enough for a second handler to be caught inside */
    for (j = 0; j < r->sr_Spin; j++)
        r->sr_Sink++;

    task = r->sr_SigTask;
    if (task)
        Signal(task, r->sr_SigMask);

    /* Counted before the run shows as completed */
    count(&r->sr_Runs);
    __atomic_store_n(&r->sr_Completed, seq, __ATOMIC_SEQ_CST);

    __atomic_store_n(&r->sr_In, 0, __ATOMIC_SEQ_CST);
    __atomic_fetch_sub(&g_Inside, 1, __ATOMIC_SEQ_CST);

    return 0;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(TickServer, volatile ULONG *, ticks)
{
    AROS_INTFUNC_INIT

    __atomic_fetch_add(ticks, 1, __ATOMIC_SEQ_CST);

    return 0;

    AROS_INTFUNC_EXIT
}

/* Part 4, in interrupt context on whichever CPU takes the interrupt */
static inline void hw_hit(struct HwSrc *h)
{
    count(&h->hs_Hits);
    if (h->hs_Task)
        Signal(h->hs_Task, h->hs_Mask);
    if (h->hs_Soft)
        Cause(&h->hs_Soft->sr_Int);
}

static AROS_INTH1(HwServer, struct HwSrc *, h)
{
    AROS_INTFUNC_INIT

    hw_hit(h);

    /* Not ours: exec goes on with the servers behind this one */
    return 0;

    AROS_INTFUNC_EXIT
}

static void HwHandler(void *data1, void *data2)
{
    hw_hit((struct HwSrc *)data1);
}

static void soft_init(struct SoftRec *r, BYTE pri, ULONG spin)
{
    memset(r, 0, sizeof(*r));
    r->sr_Int.is_Node.ln_Type = NT_INTERRUPT;
    r->sr_Int.is_Node.ln_Name = "smpsoftint";
    r->sr_Int.is_Node.ln_Pri  = pri;
    r->sr_Int.is_Code         = (VOID_FUNC)SoftHandler;
    r->sr_Int.is_Data         = r;
    r->sr_Spin                = spin;
}

/*
 * With nothing left to Cause() them: none queued, none running, and no
 * further run for a few ticks. 0 when that does not come.
 */
static int soft_idle(struct SoftRec *r, int n)
{
    ULONG runs, last = 0;
    int i, t, busy, quiet = 0;

    for (t = 0; t < WAIT_TICKS; t++)
    {
        Delay(1);
        busy = 0;
        runs = 0;
        for (i = 0; i < n; i++)
        {
            runs += rd(&r[i].sr_Runs);
            if (rd(&r[i].sr_In) || r[i].sr_Int.is_Node.ln_Type == NT_SOFTINT)
                busy = 1;
        }
        if (busy || (t && runs != last))
            quiet = 0;
        else
            quiet++;
        last = runs;
        if (quiet >= IDLE_TICKS)
            return 1;
    }
    return 0;
}

/* What the handlers of a part saw */
static void check_soft(int part, struct SoftRec *r, int n)
{
    ULONG wrong = 0, overlap = 0, reent = 0, user = 0, runs = 0, cpu = 0;
    int i;

    for (i = 0; i < n; i++)
    {
        runs    += rd(&r[i].sr_Runs);
        overlap += rd(&r[i].sr_Overlap);
        reent   += rd(&r[i].sr_Reentered);
        user    += rd(&r[i].sr_NotSuper);
        if (rd(&r[i].sr_WrongCPU))
        {
            wrong += rd(&r[i].sr_WrongCPU);
            cpu = rd(&r[i].sr_LastCPU);
        }
    }

    if (wrong)
    {
        bug("[smpsoftint] part %d: *** FAIL *** soft interrupt ran on CPU %d "
            "(%lu of %lu runs were not on CPU 0)\n",
            (int)part, (int)cpu, (unsigned long)wrong, (unsigned long)runs);
        fail++;
    }
    else
        ok(part, "every soft interrupt ran on CPU 0");

    if (overlap)
    {
        bug("[smpsoftint] part %d: *** FAIL *** two soft interrupts ran at the same time "
            "(%lu times)\n", (int)part, (unsigned long)overlap);
        fail++;
    }
    else
        ok(part, "no two soft interrupts ran at the same time");

    if (reent)
    {
        bug("[smpsoftint] part %d: *** FAIL *** a soft interrupt was entered again while "
            "it ran (%lu times)\n", (int)part, (unsigned long)reent);
        fail++;
    }
    else
        ok(part, "no soft interrupt was entered again while it ran");

    if (user)
    {
        bug("[smpsoftint] part %d: *** FAIL *** soft interrupt ran in user mode: "
            "KrnIsSuper() was false %lu times\n", (int)part, (unsigned long)user);
        fail++;
    }
    else
        ok(part, "every soft interrupt ran in supervisor mode");
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

/* One worker per CPU. Returns how many were created */
static int start_workers(int part, struct Worker *w, int n, const char *name, APTR pc)
{
    int i, created = 0;

    for (i = 0; i < n; i++)
    {
        memset(&w[i], 0, sizeof(w[i]));
        w[i].w_CPU  = i;
        w[i].w_Seen = TURN_NONE;
    }
    for (i = 0; i < n; i++)
    {
        w[i].w_Task = spawn(name, pc, i, &w[i]);
        if (w[i].w_Task == NULL)
        {
            bug("[smpsoftint] part %d: no task for CPU %d\n", (int)part, (int)i);
            continue;
        }
        created++;
        if (!wait_flag(&w[i].w_Started, WAIT_TICKS))
            bug("[smpsoftint] part %d: task on CPU %d did not start\n", (int)part, (int)i);
    }
    return created;
}

/* Stops them, with one time limit for all. Returns how many did not finish */
static int stop_workers(int part, struct Worker *w, int n)
{
    int i, s, left = 0;

    for (i = 0; i < n; i++)
        w[i].w_Stop = 1;

    for (s = 0; s < WAIT_TICKS; s++)
    {
        left = 0;
        for (i = 0; i < n; i++)
        {
            if (w[i].w_Task && !w[i].w_Done)
                left++;
        }
        if (!left)
            break;
        Delay(1);
    }

    for (i = 0; i < n; i++)
    {
        if (w[i].w_Task && !w[i].w_Done)
        {
            bug("[smpsoftint] part %d: *** FAIL *** task on CPU %d did not finish "
                "(%lu Cause() calls)\n", (int)part, (int)i, (unsigned long)w[i].w_Causes);
            fail++;
        }
    }
    if (!left)
        ok(part, "every task finished");

    return left;
}

/*
 * The one loop time is counted with: until *ctr is above 'above', or
 * maxticks VBlanks after t0, or cap iterations. Returns the iterations.
 */
static ULONG spin_until(volatile ULONG *ctr, ULONG above, ULONG t0, ULONG maxticks, ULONG cap)
{
    ULONG n;

    for (n = 0; n < cap; n++)
    {
        if (rd(ctr) > above)
            break;
        if (rd(&g_Ticks) - t0 >= maxticks)
            break;
    }
    return n;
}

/* Busy until the next VBlank. 0 when none came */
static int wait_edge(ULONG *tick)
{
    ULONG t = rd(&g_Ticks), n;

    for (n = 0; n < LOOP_CAP && rd(&g_Ticks) == t; n++)
        ;
    *tick = rd(&g_Ticks);
    return *tick != t;
}

/* Iterations of spin_until() per VBlank, 0 for none */
static ULONG calibrate(void)
{
    ULONG t0, n;

    if (!wait_edge(&t0))
        return 0;
    n = spin_until(&g_Never, 0, t0, P3_CAL_TICKS, LOOP_CAP);
    if (rd(&g_Ticks) - t0 < P3_CAL_TICKS)
        return 0;
    return n / P3_CAL_TICKS;
}

/* ---- Part 1 ---- */

static void CauseWorker(void)
{
    struct Worker *w = (struct Worker *)FindTask(NULL)->tc_UserData;
    ULONG i = 0, j, sel;

    w->w_Started = 1;

    while (!w->w_Stop)
    {
        sel = (i + w->w_CPU) & 15;
        Cause(&p1soft[(i + w->w_CPU) % NPRI].sr_Int);
        i++;
        w->w_Causes = i;

        /* Again at once, after a moment, or after a VBlank or so */
        if (sel == 0)
            Delay(1);
        else if (sel > 8)
        {
            for (j = 0; j < sel * 100; j++)
                w->w_Spin++;
        }
    }

    w->w_Done = 1;
}

/* Returns 1 when a task hung */
static int part1(int ntasks)
{
    ULONG causes = 0, runs = 0, r;
    int i, created, left, never = 0;

    for (i = 0; i < NPRI; i++)
        soft_init(&p1soft[i], softpri[i], P1_SPIN);

    created = start_workers(1, p1work, ntasks, "smpsoftint.cause", CauseWorker);
    if (!created)
    {
        bug("[smpsoftint] part 1: INVALID - no task could be created\n");
        inval++;
        return 0;
    }

    bug("[smpsoftint] part 1: %d tasks Cause() %d soft interrupts for %d seconds\n",
        (int)created, (int)NPRI, (int)(P1_TICKS / 50));
    Delay(P1_TICKS);

    left = stop_workers(1, p1work, ntasks);
    if (left)
        return 1;

    if (!soft_idle(p1soft, NPRI))
    {
        bug("[smpsoftint] part 1: *** FAIL *** a soft interrupt is still queued or running "
            "%d seconds after the last Cause()\n", (int)(WAIT_TICKS / 50));
        fail++;
    }
    else
        ok(1, "nothing left queued after the last Cause()");

    for (i = 0; i < ntasks; i++)
        causes += p1work[i].w_Causes;
    for (i = 0; i < NPRI; i++)
    {
        r = rd(&p1soft[i].sr_Runs);
        bug("[smpsoftint] part 1: priority %d: %lu runs\n", (int)softpri[i], (unsigned long)r);
        if (r == 0)
            never++;
        runs += r;
    }
    bug("[smpsoftint] part 1: %lu Cause() calls, %lu runs\n",
        (unsigned long)causes, (unsigned long)runs);

    if (never)
    {
        bug("[smpsoftint] part 1: INVALID - %d of the soft interrupts never ran\n", (int)never);
        inval++;
    }

    check_soft(1, p1soft, NPRI);

    /* A run needs a Cause() */
    if (runs > causes)
    {
        bug("[smpsoftint] part 1: *** FAIL *** more runs than Cause() calls\n");
        fail++;
    }
    else
        ok(1, "no more runs than Cause() calls");

    return 0;
}

/* ---- Part 2 ---- */

static void ServeWorker(void)
{
    struct Worker *w = (struct Worker *)FindTask(NULL)->tc_UserData;
    ULONG i = 0, j, s0, t0, n, dt;
    LONG turn;

    w->w_Started = 1;

    while (!w->w_Stop)
    {
        /* Not in a round here: main waits for this after changing the turn */
        turn = g_P2Turn;
        w->w_Seen = turn;

        if (w->w_GaveUp || (turn != TURN_ALL && turn != (LONG)w->w_CPU))
        {
            Delay(1);
            continue;
        }

        /*
         * A run numbered above s0 began after this point. If the handler
         * is running now, Cause() below asks for one more run; if the
         * interrupt is queued, its run is still to begin; else it is
         * queued now. In each case such a run has to complete.
         */
        s0 = rd(&sint.sr_Started);
        t0 = rd(&g_Ticks);
        Cause(&sint.sr_Int);
        w->w_Causes++;

        n = spin_until(&sint.sr_Completed, s0, t0, TIMEOUT_TICKS, LOOP_CAP);
        if (rd(&sint.sr_Completed) <= s0)
        {
            w->w_Timeouts++;
            w->w_GaveUp = 1;
            continue;
        }
        dt = rd(&g_Ticks) - t0;
        if (dt > w->w_WorstTicks)
            w->w_WorstTicks = dt;
        if (n > w->w_WorstIter)
            w->w_WorstIter = n;

        /* Leave the boot CPU time for more than this soft interrupt */
        i++;
        if (turn == TURN_ALL && (i & 3) == 0)
            Delay(1);
        else
        {
            for (j = 0; j < 500 + (i & 7) * 200; j++)
                w->w_Spin++;
        }
    }

    w->w_Done = 1;
}

static int wait_seen(struct Worker *w, LONG turn)
{
    int i;

    for (i = 0; i < WAIT_TICKS && w->w_Seen != turn && !w->w_GaveUp; i++)
        Delay(1);
    return w->w_Seen == turn && !w->w_GaveUp;
}

/* Returns 1 when a task hung */
static int part2(int ntasks)
{
    struct Worker *w;
    ULONG runs0, causes0, dr, dc, solo = 0, causes = 0, runs, timeouts = 0;
    int i, created, left, exact = 1, mismatch = 0;

    soft_init(&sint, 0, 200);
    g_P2Turn = TURN_NONE;

    created = start_workers(2, p2work, ntasks, "smpsoftint.serve", ServeWorker);
    if (!created)
    {
        bug("[smpsoftint] part 2: INVALID - no task could be created\n");
        inval++;
        return 0;
    }

    bug("[smpsoftint] part 2: %d tasks Cause() one soft interrupt, one at a time, "
        "then together\n", (int)created);

    /* Alone nothing is absorbed: as many runs as Cause() calls */
    for (i = 0; i < ntasks; i++)
    {
        w = &p2work[i];
        if (w->w_Task == NULL)
            continue;

        runs0   = rd(&sint.sr_Runs);
        causes0 = w->w_Causes;

        g_P2Turn = i;
        if (!wait_seen(w, i))
            exact = 0;
        else
            Delay(P2_SOLO_TICKS);
        g_P2Turn = TURN_NONE;
        if (!wait_seen(w, TURN_NONE))
            exact = 0;

        /* A run that comes late would be counted for the next CPU */
        if (!exact)
            continue;

        dr = rd(&sint.sr_Runs) - runs0;
        dc = w->w_Causes - causes0;
        solo += dc;
        if (dr != dc)
        {
            bug("[smpsoftint] part 2: *** FAIL *** %lu Cause() calls on CPU %d led to "
                "%lu runs\n", (unsigned long)dc, (int)i, (unsigned long)dr);
            fail++;
            mismatch++;
        }
    }

    if (exact && solo == 0)
    {
        bug("[smpsoftint] part 2: INVALID - no task got to call Cause() alone\n");
        inval++;
    }
    else if (exact && !mismatch)
        ok(2, "each Cause() of an idle soft interrupt led to exactly one run");
    else if (!exact)
        bug("[smpsoftint] part 2: runs not compared with Cause() calls (a task was late)\n");

    g_P2Turn = TURN_ALL;
    Delay(P2_ALL_TICKS);
    g_P2Turn = TURN_NONE;

    left = stop_workers(2, p2work, ntasks);
    if (left)
        return 1;

    if (!soft_idle(&sint, 1))
    {
        bug("[smpsoftint] part 2: *** FAIL *** the soft interrupt is still queued or "
            "running %d seconds after the last Cause()\n", (int)(WAIT_TICKS / 50));
        fail++;
    }

    runs = rd(&sint.sr_Runs);
    for (i = 0; i < ntasks; i++)
    {
        w = &p2work[i];
        if (w->w_Task == NULL)
            continue;
        causes += w->w_Causes;
        bug("[smpsoftint] part 2: CPU %d: %lu Cause() calls, worst wait %lu ticks "
            "(%lu loop iterations)\n", (int)i, (unsigned long)w->w_Causes,
            (unsigned long)w->w_WorstTicks, (unsigned long)w->w_WorstIter);
        if (w->w_Timeouts)
        {
            bug("[smpsoftint] part 2: *** FAIL *** Cause() on CPU %d was not served "
                "(no run within %d ticks)\n", (int)i, (int)TIMEOUT_TICKS);
            fail++;
            timeouts++;
        }
    }
    bug("[smpsoftint] part 2: %lu Cause() calls, %lu runs\n",
        (unsigned long)causes, (unsigned long)runs);

    if (!timeouts)
        ok(2, "every Cause() was served");

    if (runs == 0)
    {
        bug("[smpsoftint] part 2: INVALID - the soft interrupt never ran\n");
        inval++;
    }
    else if (runs > causes)
    {
        bug("[smpsoftint] part 2: *** FAIL *** more runs than Cause() calls\n");
        fail++;
    }
    else
        ok(2, "no more runs than Cause() calls");

    check_soft(2, &sint, 1);

    return 0;
}

/* ---- Part 3 ---- */

static void PromptWorker(void)
{
    struct Worker *w = (struct Worker *)FindTask(NULL)->tc_UserData;
    ULONG s0, t0, n, dt, done = 0;
    int i;

    w->w_Started = 1;

    p3cal1 = calibrate();

    for (i = 0; i < P3_ROUNDS && p3cal1; i++)
    {
        if (!wait_edge(&t0))
        {
            p3nosync++;
            break;
        }
        s0 = rd(&sint.sr_Started);
        Cause(&sint.sr_Int);
        w->w_Causes++;

        n = spin_until(&sint.sr_Completed, s0, t0, TIMEOUT_TICKS, LOOP_CAP);
        dt = rd(&g_Ticks) - t0;
        if (rd(&sint.sr_Completed) <= s0)
        {
            p3timeout++;
            break;
        }
        p3iter[done] = n;
        p3tick[done] = dt;
        done++;
        p3done = done;
    }

    if (p3cal1)
        p3cal2 = calibrate();

    w->w_Done = 1;
}

/* Returns 1 when the task hung */
static int part3(int ncpus)
{
    ULONG cal1, cal2, lo, hi, ipt, tickus, n, min = 0, max = 0, done;
    ULONG slow = 0, vslow = 0, crossed = 0;
    UQUAD sum = 0;
    int cpu = ncpus - 1, reliable, i;

    memset(&p3work, 0, sizeof(p3work));
    p3work.w_CPU = cpu;
    p3done = p3timeout = p3nosync = p3cal1 = p3cal2 = 0;

    /* The soft interrupt of part 2, idle since */
    p3work.w_Task = spawn("smpsoftint.prompt", PromptWorker, cpu, &p3work);
    if (p3work.w_Task == NULL)
    {
        bug("[smpsoftint] part 3: INVALID - no task for CPU %d\n", (int)cpu);
        inval++;
        return 0;
    }

    bug("[smpsoftint] part 3: %d times Cause() on CPU %d, each right after a VBlank\n",
        (int)P3_ROUNDS, (int)cpu);

    /* Asleep meanwhile: nothing else is running. Up to 20s for a lost one */
    if (!wait_flag(&p3work.w_Done, 4 * WAIT_TICKS))
    {
        bug("[smpsoftint] part 3: *** FAIL *** the task on CPU %d did not finish "
            "(%lu Cause() calls)\n", (int)cpu, (unsigned long)p3work.w_Causes);
        fail++;
        return 1;
    }

    soft_idle(&sint, 1);

    done = p3done;
    cal1 = p3cal1;
    cal2 = p3cal2;
    lo = (cal1 < cal2) ? cal1 : cal2;
    hi = (cal1 < cal2) ? cal2 : cal1;
    reliable = lo >= P3_CAL_MIN && hi <= lo * P3_CAL_RATIO;
    ipt = reliable ? (cal1 + cal2) / 2 : (hi ? hi : 1);
    tickus = 1000000 / (SysBase->VBlankFrequency ? SysBase->VBlankFrequency : 50);

    if (p3timeout)
    {
        bug("[smpsoftint] part 3: *** FAIL *** Cause() on CPU %d was not served "
            "(no run within %d ticks, after %lu that were)\n",
            (int)cpu, (int)TIMEOUT_TICKS, (unsigned long)done);
        fail++;
    }

    if (done == 0)
    {
        if (!p3timeout)
        {
            bug("[smpsoftint] part 3: INVALID - nothing measured (calibration %lu)\n",
                (unsigned long)cal1);
            inval++;
        }
        return 0;
    }

    for (i = 0; i < (int)done; i++)
    {
        n = p3iter[i];
        if (i == 0 || n < min)
            min = n;
        if (n > max)
            max = n;
        sum += n;

        if (p3tick[i] >= 1)
            crossed++;
        if (p3tick[i] >= P3_VSLOW_TICKS)
            vslow++;
        /* See the thresholds at the top */
        if (p3tick[i] >= 1 && (!reliable || n >= ipt / 2))
            slow++;
    }

    bug("[smpsoftint] part 3: %lu waits: min %lu, avg %lu, max %lu loop iterations\n",
        (unsigned long)done, (unsigned long)min, (unsigned long)(sum / done),
        (unsigned long)max);
    bug("[smpsoftint] part 3: about %lu, %lu, %lu microseconds (a tick of %lu is %lu "
        "iterations: %lu before, %lu after%s)\n",
        (unsigned long)((UQUAD)min * tickus / ipt),
        (unsigned long)((sum / done) * tickus / ipt),
        (unsigned long)((UQUAD)max * tickus / ipt),
        (unsigned long)tickus, (unsigned long)ipt,
        (unsigned long)cal1, (unsigned long)cal2,
        reliable ? "" : ", NOT reliable");
    bug("[smpsoftint] part 3: %lu waits ended behind the next VBlank, %lu of them slow, "
        "%lu took %d ticks or more\n", (unsigned long)crossed, (unsigned long)slow,
        (unsigned long)vslow, (int)P3_VSLOW_TICKS);

    if (slow > P3_SLOW_MAX && !reliable)
    {
        bug("[smpsoftint] part 3: INVALID - %lu of %lu waits were slow, but the timing "
            "loop is not reliable here\n", (unsigned long)slow, (unsigned long)done);
        inval++;
    }
    else if (slow > P3_SLOW_MAX)
    {
        bug("[smpsoftint] part 3: *** FAIL *** Cause() from CPU %d is only served by the "
            "next tick (%lu of %lu waits)\n",
            (int)cpu, (unsigned long)slow, (unsigned long)done);
        fail++;
    }
    else if (done < P3_ROUNDS)
    {
        if (!p3timeout)
        {
            bug("[smpsoftint] part 3: INVALID - only %lu of %d waits measured\n",
                (unsigned long)done, (int)P3_ROUNDS);
            inval++;
        }
    }
    else
        ok(3, "a Cause() on another CPU is served before the next tick");

    check_soft(3, &sint, 1);

    return 0;
}

/* ---- Part 4 ---- */

/*
 * The interrupt timer.device is driven by, where a program can know it.
 * It fires regularly, and its chain always holds timer.device's handler.
 * -1 for none. As in smpirqchurn.
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

/*
 * Both kinds of part 4 task stay, signalling nobody, until g_P4End: a
 * task that ended could still be signalled by a handler or by its peer.
 */
static void sig_task_end(struct SigTask *t, BYTE sigbit)
{
    t->st_Quiet = 1;
    while (!g_P4End)
        Delay(1);
    if (sigbit != -1)
        FreeSignal(sigbit);
    t->st_Done = 1;
}

static BYTE sig_task_begin(struct SigTask *t)
{
    BYTE sigbit = AllocSignal(-1);

    if (sigbit == -1)
        t->st_NoSig = 1;
    else
        t->st_Mask = 1UL << sigbit;
    t->st_Task = FindTask(NULL);
    t->st_Ready = 1;

    while (!g_P4Go && !g_P4Stop)
        Delay(1);

    return sigbit;
}

/* Woken from the interrupt, or from the soft interrupt */
static void WaiterTask(void)
{
    struct SigTask *t = (struct SigTask *)FindTask(NULL)->tc_UserData;
    BYTE sigbit = sig_task_begin(t);

    if (sigbit != -1)
    {
        while (!g_P4Stop)
        {
            Wait(t->st_Mask);
            t->st_Count++;
        }
    }

    sig_task_end(t, sigbit);
}

/* Two of these on a CPU wake each other: a task switch every time */
static void PingPongTask(void)
{
    struct SigTask *t = (struct SigTask *)FindTask(NULL)->tc_UserData;
    struct SigTask *peer = t->st_Peer;
    BYTE sigbit = sig_task_begin(t);

    /* Main lets no pair go that is not complete */
    if (sigbit != -1 && g_P4Go)
    {
        while (!g_P4Stop)
        {
            Signal(peer->st_Task, peer->st_Mask);
            Wait(t->st_Mask);
            t->st_Count++;
        }
    }

    sig_task_end(t, sigbit);
}

static struct SigTask *p4_task(int idx, int ntasks)
{
    if (idx < 2 * ntasks)
        return &pptask[idx / 2][idx & 1];
    return (idx == 2 * ntasks) ? &waita : &waitb;
}

/* Returns 1 when something stopped or hung */
static int part4(int ncpus, int ntasks)
{
    struct SigTask *t;
    ULONG cur[MAXTASKS + P4_EXTRA], last[MAXTASKS + P4_EXTRA], total, probe;
    int stall[MAXTASKS + P4_EXTRA], failed[MAXTASKS + P4_EXTRA];
    int nall = 2 * ntasks + 2, ncnt = ntasks + P4_EXTRA;
    int i, s, k, slowest, bad = 0, stopped = 0, vert = 0, left, idle = 0;
    LONG irq;
    APTR kh = NULL;

    bug("[smpsoftint] part 4: running\n");

    g_P4Go = g_P4Stop = g_P4End = 0;
    memset(pptask, 0, sizeof(pptask));
    memset(&waita, 0, sizeof(waita));
    memset(&waitb, 0, sizeof(waitb));
    memset(&hwvert, 0, sizeof(hwvert));
    memset(&hwkirq, 0, sizeof(hwkirq));
    memset(stall, 0, sizeof(stall));
    memset(failed, 0, sizeof(failed));
    memset(last, 0, sizeof(last));
    soft_init(&p4soft[0], 0, 200);
    soft_init(&p4soft[1], 16, 200);

    for (i = 0; i < ntasks; i++)
    {
        pptask[i][0].st_Peer = &pptask[i][1];
        pptask[i][1].st_Peer = &pptask[i][0];
    }

    /* Both waiters away from the boot CPU, where the soft interrupts run */
    waita.st_Created = spawn("smpsoftint.waita", WaiterTask, ncpus - 1, &waita);
    waitb.st_Created = spawn("smpsoftint.waitb", WaiterTask, 1, &waitb);
    for (i = 0; i < 2 * ntasks; i++)
    {
        t = p4_task(i, ntasks);
        t->st_Created = spawn("smpsoftint.pingpong", PingPongTask, i / 2, t);
    }

    for (i = 0; i < nall; i++)
    {
        t = p4_task(i, ntasks);
        if (t->st_Created == NULL || !wait_flag(&t->st_Ready, WAIT_TICKS) || t->st_NoSig)
            bad++;
    }

    if (bad)
    {
        bug("[smpsoftint] part 4: INVALID - %d of %d tasks did not start or got no signal\n",
            (int)bad, (int)nall);
        inval++;
    }
    else
    {
        /* Every task is waiting for g_P4Go and has its signal: now the sources */
        p4soft[0].sr_SigTask = waitb.st_Task;
        p4soft[0].sr_SigMask = waitb.st_Mask;
        p4soft[1].sr_SigTask = waitb.st_Task;
        p4soft[1].sr_SigMask = waitb.st_Mask;

        irq = live_irq();
        if (irq >= 0)
        {
            /* Is it really firing? */
            kh = KrnAddIRQHandler((ULONG)irq, (APTR)HwHandler, &hwkirq, NULL);
            if (kh)
            {
                Delay(25);
                KrnRemIRQHandler(kh);
                kh = NULL;
                /* A pass over the chain may still be in flight elsewhere */
                Delay(1);
                probe = rd(&hwkirq.hs_Hits);
                if (probe < 3)
                {
                    bug("[smpsoftint] part 4: IRQ %d does not fire here (%lu times in "
                        "half a second): VBlank only\n", (int)irq, (unsigned long)probe);
                    irq = -1;
                }
            }
            else
            {
                bug("[smpsoftint] part 4: KrnAddIRQHandler(%d) failed: VBlank only\n", (int)irq);
                irq = -1;
            }
        }
        else
            bug("[smpsoftint] part 4: no known live IRQ on this port: VBlank only\n");

        hwvert.hs_Int.is_Node.ln_Type = NT_INTERRUPT;
        hwvert.hs_Int.is_Node.ln_Name = "smpsoftint.vblank";
        hwvert.hs_Int.is_Node.ln_Pri  = 0;
        hwvert.hs_Int.is_Code         = (VOID_FUNC)HwServer;
        hwvert.hs_Int.is_Data         = &hwvert;
        hwvert.hs_Task                = waita.st_Task;
        hwvert.hs_Mask                = waita.st_Mask;
        hwvert.hs_Soft                = &p4soft[0];
        AddIntServer(INTB_VERTB, &hwvert.hs_Int);
        vert = 1;

        if (irq >= 0)
        {
            hwkirq.hs_Hits = 0;
            hwkirq.hs_Task = waita.st_Task;
            hwkirq.hs_Mask = waita.st_Mask;
            hwkirq.hs_Soft = &p4soft[1];
            kh = KrnAddIRQHandler((ULONG)irq, (APTR)HwHandler, &hwkirq, NULL);
            if (kh)
                bug("[smpsoftint] part 4: VBlank server and a handler on IRQ %d\n", (int)irq);
        }

        g_P4Go = 1;

        for (s = 1; s <= P4_SECS && !stopped; s++)
        {
            Delay(50);

            total = 0;
            slowest = 0;
            for (i = 0; i < ntasks; i++)
            {
                cur[i] = pptask[i][0].st_Count + pptask[i][1].st_Count;
                total += cur[i] - last[i];
                if (cur[i] - last[i] < cur[slowest] - last[slowest])
                    slowest = i;
            }
            cur[ntasks]     = rd(&hwvert.hs_Hits);
            cur[ntasks + 1] = waita.st_Count;
            cur[ntasks + 2] = waitb.st_Count;
            cur[ntasks + 3] = rd(&hwkirq.hs_Hits);

            bug("[smpsoftint] part 4: %ds: vblank %lu, irq %lu, soft interrupt runs %lu, "
                "wakes %lu and %lu, task switches +%lu (least: CPU %d +%lu)\n",
                (int)s, (unsigned long)cur[ntasks], (unsigned long)cur[ntasks + 3],
                (unsigned long)(rd(&p4soft[0].sr_Runs) + rd(&p4soft[1].sr_Runs)),
                (unsigned long)cur[ntasks + 1], (unsigned long)cur[ntasks + 2],
                (unsigned long)total, (int)slowest,
                (unsigned long)(cur[slowest] - last[slowest]));

            for (k = 0; k < ncnt; k++)
            {
                /* The IRQ handler's counter only where there is one */
                if (k == ntasks + 3 && kh == NULL)
                    continue;

                if (cur[k] != last[k])
                    stall[k] = 0;
                else if (++stall[k] >= P4_STALL && !failed[k])
                {
                    failed[k] = 1;
                    stopped = 1;
                    fail++;
                    if (k < ntasks)
                        bug("[smpsoftint] part 4: *** FAIL *** CPU %d stopped: its two tasks "
                            "did not wake each other for %d seconds\n", (int)k, (int)P4_STALL);
                    else if (k == ntasks)
                        bug("[smpsoftint] part 4: *** FAIL *** interrupts stopped: no VBlank "
                            "for %d seconds\n", (int)P4_STALL);
                    else if (k == ntasks + 1)
                        bug("[smpsoftint] part 4: *** FAIL *** the waiter signalled by the "
                            "interrupt was not woken for %d seconds\n", (int)P4_STALL);
                    else if (k == ntasks + 2)
                        bug("[smpsoftint] part 4: *** FAIL *** the waiter signalled by the "
                            "soft interrupt was not woken for %d seconds\n", (int)P4_STALL);
                    else
                        bug("[smpsoftint] part 4: *** FAIL *** interrupts stopped: IRQ %d "
                            "did not fire for %d seconds\n", (int)irq, (int)P4_STALL);
                }
                last[k] = cur[k];
            }
        }
    }

    /* What follows may not come back when something stopped */
    if (stopped)
        verdict();

    /*
     * 1. the tasks stop signalling (they are woken for it, and stay),
     * 2. the interrupt sources go, from here and never from a handler,
     * 3. the soft interrupts they queued run out,
     * 4. the tasks end.
     */
    g_P4Stop = 1;
    for (i = 0; i < nall; i++)
    {
        t = p4_task(i, ntasks);
        if (t->st_Ready && !t->st_NoSig)
            Signal(t->st_Task, t->st_Mask);
    }

    for (s = 0; s < WAIT_TICKS; s++)
    {
        left = 0;
        for (i = 0; i < nall; i++)
        {
            t = p4_task(i, ntasks);
            if (t->st_Created && !t->st_Quiet)
                left++;
        }
        if (!left)
            break;
        Delay(1);
    }

    if (kh)
        KrnRemIRQHandler(kh);
    if (vert)
        RemIntServer(INTB_VERTB, &hwvert.hs_Int);

    Delay(2);
    idle = soft_idle(p4soft, 2);

    /* A task still signalling keeps the others: they are never freed then */
    if (!left && idle)
    {
        g_P4End = 1;
        for (s = 0; s < WAIT_TICKS; s++)
        {
            left = 0;
            for (i = 0; i < nall; i++)
            {
                t = p4_task(i, ntasks);
                if (t->st_Created && !t->st_Done)
                    left++;
            }
            if (!left)
                break;
            Delay(1);
        }
    }

    if (stopped)
    {
        bug("[smpsoftint] part 4: stopped and cleaned up (%d tasks left)\n", (int)left);
        return 1;
    }

    if (left)
    {
        bug("[smpsoftint] part 4: *** FAIL *** %d of %d tasks did not stop\n",
            (int)left, (int)nall);
        fail++;
    }
    else if (idle)
        ok(4, "every task finished");

    if (!idle)
    {
        bug("[smpsoftint] part 4: *** FAIL *** a soft interrupt is still queued or running "
            "%d seconds after its interrupt was removed\n", (int)(WAIT_TICKS / 50));
        fail++;
    }

    if (bad)
        return left || !idle;

    ok(4, "every CPU kept switching tasks");
    ok(4, "the interrupts kept being served");
    ok(4, "the tasks signalled from the interrupt and the soft interrupt kept being woken");

    if (rd(&p4soft[0].sr_Runs) == 0 || (kh && rd(&p4soft[1].sr_Runs) == 0))
    {
        bug("[smpsoftint] part 4: INVALID - a soft interrupt never ran\n");
        inval++;
    }
    check_soft(4, p4soft, kh ? 2 : 1);

    return left || !idle;
}

int main(void)
{
    int ncpus, ntasks, hung;
    ULONG t0;

    KernelBase = OpenResource("kernel.resource");
    ncpus = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smpsoftint] start: cpus=%d\n", (int)ncpus);
    if (ncpus < 2)
    {
        bug("[smpsoftint] only %d CPU online - needs SMP; SKIP\n", (int)ncpus);
        return RETURN_WARN;
    }
    ntasks = (ncpus < MAXTASKS) ? ncpus : MAXTASKS;

    tickserver.is_Node.ln_Type = NT_INTERRUPT;
    tickserver.is_Node.ln_Name = "smpsoftint.tick";
    tickserver.is_Node.ln_Pri  = -32;
    tickserver.is_Code         = (VOID_FUNC)TickServer;
    tickserver.is_Data         = (APTR)&g_Ticks;
    AddIntServer(INTB_VERTB, &tickserver);

    t0 = rd(&g_Ticks);
    Delay(25);
    if (rd(&g_Ticks) == t0)
    {
        RemIntServer(INTB_VERTB, &tickserver);
        bug("[smpsoftint] INVALID: the VBlank server never ran\n");
        bug("[smpsoftint] DONE: 0 PASS, 0 FAIL, 1 INVALID\n");
        return RETURN_WARN;
    }

    hung = part1(ntasks);

    if (!hung)
        hung = part2(ntasks);
    else
        bug("[smpsoftint] part 2: not run after a hang\n");

    if (!hung)
        hung = part3(ncpus);
    else
        bug("[smpsoftint] part 3: not run after a hang\n");

    if (!hung)
        part4(ncpus, ntasks);
    else
        bug("[smpsoftint] part 4: not run after a hang\n");

    /* First the verdict: after a hang the removal below may not come back */
    verdict();

    RemIntServer(INTB_VERTB, &tickserver);
    /* The chain may be walked on another CPU, and the tasks are just leaving */
    Delay(5);

    return fail ? RETURN_FAIL : (inval ? RETURN_WARN : RETURN_OK);
}
#else
int main(void)
{
    return RETURN_WARN;
}
#endif
