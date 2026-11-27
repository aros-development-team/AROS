/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Hardware interrupt affinity.

          An interrupt is delivered to one CPU: the boot CPU, until
          KrnModifyIRQA() is given KERNELTAG_IRQ_AFFINITY for it, which
          takes TASKAFFINITY_BOOT, TASKAFFINITY_ANY or a CPU mask, picks
          the least loaded of the CPUs named, once, and has the interrupt
          delivered there from then on. KrnGetIRQA() tells the CPU. So:
            - the handlers of an interrupt run on the CPU it was given
              to, on no other, and never on two at the same time,
            - an interrupt that is moved while it fires keeps being
              served,
            - a request that cannot be carried out returns FALSE and
              changes nothing,
            - soft interrupts caused by a moved handler still run on
              the boot CPU.

          The test adds a handler of its own to an interrupt that fires
          by itself or can be made to:
            pc      IRQ 0, the PIT, beside timer.device's handler.
            riscv   the PLIC source of the PCI host bridge that raises
                    interrupts while files are read (the boot disk is
                    behind it); found by trying sources 32 to 35.
            Pi 4    compare channel 3 of the BCM2835 system timer, which
                    nothing else uses: GIC interrupt 96 + 3. Where that
                    does not fire, the SD host's interrupt (158) if file
                    reads raise it.
            Pi 3    IRQ 1, timer.device's. The controller routes all its
                    interrupts together, so only the boot CPU can be had
                    and only that is checked.
          Where no interrupt fires the calls are still checked, without
          looking at where the handler runs.

          Part 1: before any change KrnGetIRQA() reports CPU 0 and the
          handler runs on CPU 0 only.

          Part 2: the interrupt is given to CPU 1, to one in the middle
          and to the last one, and back to the boot CPU. Each time
          KrnGetIRQA() reports that CPU and, after a moment, every call
          of the handler is there.

          Part 3: with a busy task on every CPU but one, TASKAFFINITY_ANY
          and a mask of a busy CPU and the idle one choose the idle one.

          Part 4: an empty mask, TASKAFFINITY_ALL_BUT_SELF, an interrupt
          that does not exist and one that is private to each CPU are
          refused, and KrnGetIRQA() answers as before.

          Part 5: a task moves the interrupt between two CPUs, then
          between all of them, as fast as the calls return, while it
          fires. The handler is never entered on two CPUs at once, the
          interrupt keeps being served and every call returns.

          Part 6: with the interrupt on the last CPU the handler calls
          Cause(); every run of that soft interrupt is on CPU 0.

          The handler touches static data and atomic counters only. The
          interrupt is put back on the boot CPU and the handler removed
          before the program ends, whatever happened.

          Moving IRQ 0 on pc moves timer.device's handler with ours. If
          the interrupt is lost there the clock stops and Delay() never
          returns, so on pc the test waits by looking at the clock in a
          loop with a limit, and gives up with a verdict when the limit
          is reached.
*/

#include <aros/config.h>
#include <aros/asmcall.h>
#include <aros/kernel.h>

#include <exec/interrupts.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <resources/kernel.h>
#include <resources/task.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/kernel.h>
#include <proto/task.h>

#include <stdio.h>
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

#define STACKSIZE       16384
#define WAIT_TICKS      250             /* 5s */
#define MAXCPUS         64

#define SETTLE_TICKS    10              /* after a move, before the calls are counted */
#define WATCH_TICKS     50              /* the calls of a second are counted */
#define LOAD_TICKS      125             /* KATTR_CPULoad is worked out about once a second */
#define P5_SECS         5               /* part 5: the interrupt is moved this long */
#define P5_ALL          2               /* part 5: between all CPUs from this second on */
#define P6_TICKS        75              /* part 6: the handler calls Cause() this long */
#define IDLE_TICKS      3
#define BOOT_TRIES      50              /* putting the interrupt back at the end */

#define MIN_HITS        5               /* a second of an interrupt that fires by itself */
#define MIN_IOHITS      3               /* two seconds of file reads */
#define PROBE_TICKS     100
#define IO_CHUNK        32768
#define MAXCAND         4

#define NOCPU           ((IPTR)0x7FFFFFF0)      /* what KrnGetIRQA() is handed */
#define BAD_IRQ         0x00100000UL            /* no port has this many interrupts */

/* What makes the interrupt fire */
#define SRC_NONE        0               /* nothing: only the calls are checked */
#define SRC_TIMER       1               /* fires by itself */
#define SRC_OWNED       2               /* a timer channel the handler re-arms */
#define SRC_IO          3               /* a disk controller: fires while files are read */

#if defined(__aarch64__)
/* BCM2835 system timer, in 32-bit registers from peripheral base + 0x3000 */
#define ST_OFFSET       0x3000
#define ST_CS           0               /* +0x00: match bits, written 1 to clear */
#define ST_CLO          1               /* +0x04: the 1MHz counter */
#define ST_C3           6               /* +0x18: compare 3 */
#define ST_M3           (1UL << 3)
#define ST_INTERVAL     10000           /* 100 a second */
#define ST_RATE         100
#define BCM2711_BASE    0xFE000000UL
#define BCM2712_BASE    0x107C000000ULL
#define BCM2711_GPUIRQ  96              /* the legacy interrupts through the GIC */
#define BCM2711_SDIRQ   (BCM2711_GPUIRQ + 62)
#define GIC_TIMER_PPI   30              /* the per-CPU generic timer */
#endif

struct Source
{
    LONG                s_IRQ;          /* -1: none */
    int                 s_Kind;
    int                 s_Movable;      /* the port can deliver it to any CPU */
    int                 s_Clock;        /* the system clock stops without it */
    ULONG               s_Rate;         /* it fires at least this often a second; 0: not known */
    LONG                s_Private;      /* an interrupt private to each CPU, -1 for none */
    const char         *s_Name;
};

struct Snap
{
    ULONG               sn_Hits[MAXCPUS + 1];
    ULONG               sn_Total;
};

struct Busy
{
    struct Task        *b_Task;
    volatile ULONG      b_Stop;
    volatile ULONG      b_Started;
    volatile ULONG      b_Done;
    volatile ULONG      b_Spin;
};

struct Mover
{
    struct Task        *m_Task;
    ULONG               m_IRQ;
    ULONG               m_A, m_B;       /* the two CPUs of the first phase */
    ULONG               m_NCPUs;
    volatile ULONG      m_Phase;
    volatile ULONG      m_Stop;
    volatile ULONG      m_Started;
    volatile ULONG      m_Done;
    volatile ULONG      m_Calls;
    volatile ULONG      m_Moved;        /* KrnModifyIRQA() returned TRUE */
    volatile ULONG      m_Refused;      /* it returned FALSE */
};

static struct Source    src = { -1, SRC_NONE, 0, 0, 0, -1, "none" };

/* All the handlers touch. Never freed: a task that hangs keeps using its own */
static volatile ULONG   g_Hits[MAXCPUS + 1];    /* handler calls by CPU; last: CPU number too high */
static volatile ULONG   g_In;                   /* the handler is inside */
static volatile ULONG   g_Overlap;              /* it was entered while it was inside */
static volatile ULONG   g_SoftOn;               /* part 6: the handler calls Cause() */
static volatile ULONG   g_SoftRuns, g_SoftWrong, g_SoftLastCPU, g_SoftIn;
#if defined(__riscv) || defined(__aarch64__)
static volatile ULONG   g_Probe[MAXCAND];
#endif
static struct Interrupt softint;
#if defined(__aarch64__)
static volatile ULONG * volatile g_SysTimer;    /* set while compare 3 is ours */
#endif

static struct Busy      busy[MAXCPUS];
static struct Mover     mover;

static cpumask_t       *g_Mask[MAXCPUS];        /* one CPU each */
static APTR             g_Handle;
static int              live;                   /* our handler is in and the interrupt fires */
static int              hung;                   /* a call did not return */
static int              clock_dead;             /* the clock stopped: no Delay() any more */
static ULONG            g_SpinPerTick;

static BPTR                     io_dir, io_fh;
static struct FileInfoBlock    *io_fib;
static int                      io_diridx;
static ULONG                    g_Reads;        /* Read() calls that returned data */
static UBYTE                    io_buf[IO_CHUNK];
static const char * const       io_dirs[] = { "SYS:C", "SYS:Libs", "SYS:Devs", "SYS:Classes" };
#define IO_DIRS         (sizeof(io_dirs) / sizeof(io_dirs[0]))

static int pass, fail, inval, verdict_given;
static char what[192], label[96];

static void ok(int part, const char *text)
{
    bug("[smpirqaff] part %d: OK - %s\n", (int)part, text);
    pass++;
}

/* First the verdict where what follows may not come back */
static void verdict(void)
{
    if (!verdict_given)
        bug("[smpirqaff] DONE: %d PASS, %d FAIL, %d INVALID\n",
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
 * The handler under test. Runs in interrupt context on the CPU the
 * interrupt is delivered to. An interrupt is served by one CPU at a
 * time, so g_In is 0 on entry, or the handler is running on a second
 * CPU.
 */
static void AffHandler(void *data1, void *data2)
{
    ULONG cpu = KrnGetCPUNumber();
#if defined(__aarch64__)
    volatile ULONG *st = __atomic_load_n(&g_SysTimer, __ATOMIC_SEQ_CST);
#endif

    if (__atomic_exchange_n(&g_In, 1, __ATOMIC_SEQ_CST) != 0)
        count(&g_Overlap);
    count(&g_Hits[(cpu < MAXCPUS) ? cpu : MAXCPUS]);

#if defined(__aarch64__)
    /* Our own timer channel: take the match back and ask for the next */
    if (st)
    {
        st[ST_CS] = ST_M3;
        st[ST_C3] = st[ST_CLO] + ST_INTERVAL;
    }
#endif

    if (rd(&g_SoftOn))
        Cause(&softint);

    __atomic_store_n(&g_In, 0, __ATOMIC_SEQ_CST);
}

/* Part 6. On SMP a soft interrupt is served by the boot CPU alone */
static AROS_INTH1(SoftHandler, APTR, unused)
{
    AROS_INTFUNC_INIT

    ULONG cpu = KrnGetCPUNumber();

    __atomic_store_n(&g_SoftIn, 1, __ATOMIC_SEQ_CST);
    if (cpu != 0)
    {
        __atomic_store_n(&g_SoftLastCPU, cpu, __ATOMIC_SEQ_CST);
        count(&g_SoftWrong);
    }
    count(&g_SoftRuns);
    __atomic_store_n(&g_SoftIn, 0, __ATOMIC_SEQ_CST);

    return 0;

    AROS_INTFUNC_EXIT
}

#if defined(__riscv) || defined(__aarch64__)
/* Finding the interrupt that file reads raise */
static void CountHandler(void *data1, void *data2)
{
    __atomic_fetch_add((volatile ULONG *)data1, 1, __ATOMIC_SEQ_CST);
}
#endif

static void BusyWorker(void)
{
    struct Busy *b = (struct Busy *)FindTask(NULL)->tc_UserData;

    b->b_Started = 1;
    while (!b->b_Stop)
        b->b_Spin++;
    b->b_Done = 1;
}

/* Part 5: the interrupt from CPU to CPU, as fast as the calls return */
static void MoverWorker(void)
{
    struct Mover *m = (struct Mover *)FindTask(NULL)->tc_UserData;
    struct TagItem tags[2];
    ULONG i = 0, cpu;

    tags[0].ti_Tag  = KERNELTAG_IRQ_AFFINITY;
    tags[1].ti_Tag  = TAG_DONE;
    tags[1].ti_Data = 0;

    m->m_Started = 1;

    while (!m->m_Stop)
    {
        if (m->m_Phase == 0)
            cpu = (i & 1) ? m->m_A : m->m_B;
        else
            cpu = i % m->m_NCPUs;
        i++;

        tags[0].ti_Data = (IPTR)g_Mask[cpu];
        if (KrnModifyIRQA(m->m_IRQ, tags))
            m->m_Moved++;
        else
            m->m_Refused++;
        m->m_Calls = i;
    }

    m->m_Done = 1;
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

/* The affinity of an interrupt: KrnModifyIRQA()'s answer */
static ULONG irq_set(ULONG irq, IPTR affinity)
{
    struct TagItem tags[2];

    tags[0].ti_Tag  = KERNELTAG_IRQ_AFFINITY;
    tags[0].ti_Data = affinity;
    tags[1].ti_Tag  = TAG_DONE;
    tags[1].ti_Data = 0;

    return KrnModifyIRQA(irq, tags);
}

/* The CPU KrnGetIRQA() reports, NOCPU when it leaves the tag alone */
static IPTR irq_cpu(ULONG irq)
{
    struct TagItem tags[2];

    tags[0].ti_Tag  = KERNELTAG_IRQ_AFFINITY;
    tags[0].ti_Data = NOCPU;
    tags[1].ti_Tag  = TAG_DONE;
    tags[1].ti_Data = 0;

    KrnGetIRQA(irq, tags);

    return tags[0].ti_Data;
}

static ULONG now_ticks(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return (ULONG)ds.ds_Days * 4320000UL + (ULONG)ds.ds_Minute * 3000UL + (ULONG)ds.ds_Tick;
}

/* Looks at the clock until 'ticks' have passed, 'cap' times at most (0: no limit) */
static UQUAD spin_ticks(ULONG ticks, UQUAD cap)
{
    ULONG start = now_ticks();
    UQUAD n = 0;

    while (now_ticks() - start < ticks)
    {
        n++;
        if (cap && n >= cap)
            break;
    }
    return n;
}

/*
 * Waits. Where the interrupt under test is the one the clock runs from,
 * Delay() would never return once the interrupt is lost: there the
 * clock is watched in a loop, eight times as long as the wait took when
 * the loop was calibrated; after that the clock is taken to have
 * stopped, and nothing waits any more.
 */
static void nap(ULONG ticks)
{
    UQUAD cap;

    if (clock_dead)
        return;
    if (!src.s_Clock || !g_SpinPerTick)
    {
        Delay(ticks);
        return;
    }

    cap = (UQUAD)g_SpinPerTick * ticks * 8 + 100000;
    if (spin_ticks(ticks, cap) >= cap)
    {
        clock_dead = 1;
        bug("[smpirqaff] the clock stopped: %lu ticks did not pass\n", (unsigned long)ticks);
    }
}

/*
 * Waits for a flag with Delay(). For use while the interrupt under test is
 * on the boot CPU, where the clock is safe to wait for: unlike nap(), this
 * gives the CPU up, which a task started on the caller's CPU needs.
 */
static int sleep_flag(volatile ULONG *flag, int ticks)
{
    int i;

    for (i = 0; i < ticks && !*flag; i++)
        Delay(1);
    return *flag != 0;
}

static int wait_flag(volatile ULONG *flag, int ticks)
{
    int i;

    for (i = 0; i < ticks && !*flag; i++)
    {
        /* Without a clock: about as long as a tick took */
        if (clock_dead)
            spin_ticks(1, g_SpinPerTick);
        else
            nap(1);
    }
    return *flag != 0;
}

/* The next file of the directories read, opened; BNULL if there is none */
static BPTR io_next(void)
{
    BPTR fh, old;
    int tries;

    if (io_fib == NULL)
        return BNULL;

    for (tries = 0; tries < 4096; tries++)
    {
        if (!io_dir)
        {
            io_dir = Lock((CONST_STRPTR)io_dirs[io_diridx], SHARED_LOCK);
            io_diridx = (io_diridx + 1) % IO_DIRS;
            if (!io_dir)
                continue;
            if (!Examine(io_dir, io_fib))
            {
                UnLock(io_dir);
                io_dir = BNULL;
                continue;
            }
        }

        if (!ExNext(io_dir, io_fib))
        {
            UnLock(io_dir);
            io_dir = BNULL;
            continue;
        }
        if (io_fib->fib_DirEntryType >= 0)
            continue;

        old = CurrentDir(io_dir);
        fh = Open(io_fib->fib_FileName, MODE_OLDFILE);
        CurrentDir(old);
        if (fh)
            return fh;
    }
    return BNULL;
}

static void io_end(void)
{
    if (io_fh)
        Close(io_fh);
    if (io_dir)
        UnLock(io_dir);
    if (io_fib)
        FreeDosObject(DOS_FIB, io_fib);
    io_fh = BNULL;
    io_dir = BNULL;
    io_fib = NULL;
}

/*
 * Lets the interrupt fire for 'ticks'. One that fires by itself is
 * waited for. A disk controller's is raised by reading: one file after
 * another, so that what is read is not what a cache still holds.
 */
static void stir(ULONG ticks)
{
    ULONG start;
    LONG n;

    if (src.s_Kind != SRC_IO)
    {
        nap(ticks);
        return;
    }

    start = now_ticks();
    do
    {
        if (!io_fh && !(io_fh = io_next()))
        {
            Delay(1);
            continue;
        }
        n = Read(io_fh, io_buf, IO_CHUNK);
        if (n > 0)
            g_Reads++;
        else
        {
            Close(io_fh);
            io_fh = BNULL;
        }
    } while (now_ticks() - start < ticks);
}

/* The calling task on one CPU; cpu < 0: on any again */
static void pin_self(int cpu)
{
    if (!TaskResBase)
        return;
    if (cpu < 0)
        SetTaskAffinity(NULL, (APTR)TASKAFFINITY_ANY);
    else
        SetTaskAffinity(NULL, g_Mask[cpu]);
}

static void snap(struct Snap *s)
{
    int i;

    s->sn_Total = 0;
    for (i = 0; i <= MAXCPUS; i++)
    {
        s->sn_Hits[i] = rd(&g_Hits[i]);
        s->sn_Total += s->sn_Hits[i];
    }
}

static ULONG total_hits(void)
{
    struct Snap s;

    snap(&s);
    return s.sn_Total;
}

/*
 * Where does the handler run? After a moment, for an interrupt taken
 * just before it was moved, every call of a second has to be on 'cpu'.
 * Returns 1 when it is so.
 */
static int check_delivery(int part, int cpu)
{
    struct Snap a, b;
    ULONG on, off, reads;
    int i, other = -1;

    stir(SETTLE_TICKS);
    snap(&a);
    reads = g_Reads;
    stir(WATCH_TICKS);
    snap(&b);
    reads = g_Reads - reads;

    if (clock_dead)
    {
        bug("[smpirqaff] part %d: *** FAIL *** the clock stopped with IRQ %d on CPU %d: "
            "the interrupt is not served any more\n", (int)part, (int)src.s_IRQ, (int)cpu);
        fail++;
        return 0;
    }

    on  = b.sn_Hits[cpu] - a.sn_Hits[cpu];
    off = (b.sn_Total - a.sn_Total) - on;
    for (i = 0; i <= MAXCPUS; i++)
    {
        if (i != cpu && b.sn_Hits[i] != a.sn_Hits[i])
        {
            other = i;
            break;
        }
    }

    bug("[smpirqaff] part %d: IRQ %d on CPU %d: %lu handler calls there, %lu elsewhere\n",
        (int)part, (int)src.s_IRQ, (int)cpu, (unsigned long)on, (unsigned long)off);

    if (off)
    {
        bug("[smpirqaff] part %d: *** FAIL *** with IRQ %d on CPU %d the handler ran %lu times "
            "on other CPUs (CPU %d among them)\n",
            (int)part, (int)src.s_IRQ, (int)cpu, (unsigned long)off, (int)other);
        fail++;
        return 0;
    }
    if (on)
    {
        snprintf(what, sizeof(what), "every call of the handler was on CPU %d", cpu);
        ok(part, what);
        return 1;
    }
    if (src.s_Kind == SRC_IO && reads)
    {
        bug("[smpirqaff] part %d: INVALID - no interrupt while %lu reads completed "
            "(nothing was read from the disk)\n", (int)part, (unsigned long)reads);
        inval++;
        return 0;
    }

    bug("[smpirqaff] part %d: *** FAIL *** IRQ %d is not served on CPU %d: no handler call "
        "in a second\n", (int)part, (int)src.s_IRQ, (int)cpu);
    fail++;
    return 0;
}

/*
 * Our handler into the interrupt, and a look whether it fires. Returns
 * the calls counted, -1 when the handler could not be added.
 */
static LONG go_live(ULONG ticks)
{
    ULONG n;
    int i;

    for (i = 0; i <= MAXCPUS; i++)
        __atomic_store_n(&g_Hits[i], 0, __ATOMIC_SEQ_CST);

#if defined(__aarch64__)
    if (src.s_Kind == SRC_OWNED)
    {
        IPTR base = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);
        volatile ULONG *st = (volatile ULONG *)(base + ST_OFFSET);

        /* A match left from before must not be the first interrupt */
        st[ST_CS] = ST_M3;
        __atomic_store_n(&g_SysTimer, st, __ATOMIC_SEQ_CST);
    }
#endif

    g_Handle = KrnAddIRQHandler((ULONG)src.s_IRQ, (APTR)AffHandler, NULL, NULL);
    if (g_Handle == NULL)
    {
#if defined(__aarch64__)
        __atomic_store_n(&g_SysTimer, NULL, __ATOMIC_SEQ_CST);
#endif
        return -1;
    }

#if defined(__aarch64__)
    if (src.s_Kind == SRC_OWNED)
    {
        volatile ULONG *st = __atomic_load_n(&g_SysTimer, __ATOMIC_SEQ_CST);

        st[ST_C3] = st[ST_CLO] + ST_INTERVAL;
    }
#endif

    stir(ticks);
    n = total_hits();
    bug("[smpirqaff] IRQ %d (%s): %lu interrupts in %lu ticks\n",
        (int)src.s_IRQ, src.s_Name, (unsigned long)n, (unsigned long)ticks);

    return (LONG)n;
}

/* Our handler out again. The last handler of an interrupt masks it */
static void drop_live(void)
{
    if (g_Handle)
        KrnRemIRQHandler(g_Handle);
    g_Handle = NULL;
    live = 0;

#if defined(__aarch64__)
    {
        volatile ULONG *st = __atomic_load_n(&g_SysTimer, __ATOMIC_SEQ_CST);

        if (st)
        {
            st[ST_CS] = ST_M3;
            __atomic_store_n(&g_SysTimer, NULL, __ATOMIC_SEQ_CST);
        }
    }
#endif
}

#if defined(__riscv) || defined(__aarch64__)
/*
 * The interrupt that reading files raises: a counting handler on every
 * candidate, two seconds of reading, and the one that counted most. A
 * candidate nobody else has a handler on is unmasked for that time and
 * masked again when our handler goes. -1 if none counted.
 */
static LONG probe_io(const UWORD *cand, int n)
{
    APTR handle[MAXCAND];
    ULONG hits, best = 0;
    LONG irq = -1;
    int i;

    if (n > MAXCAND)
        n = MAXCAND;

    for (i = 0; i < n; i++)
    {
        __atomic_store_n(&g_Probe[i], 0, __ATOMIC_SEQ_CST);
        handle[i] = KrnAddIRQHandler(cand[i], (APTR)CountHandler, (APTR)&g_Probe[i], NULL);
    }

    stir(PROBE_TICKS);

    for (i = 0; i < n; i++)
    {
        if (handle[i])
            KrnRemIRQHandler(handle[i]);
        hits = rd(&g_Probe[i]);
        bug("[smpirqaff] IRQ %d: %lu interrupts while files were read%s\n",
            (int)cand[i], (unsigned long)hits, handle[i] ? "" : " (no handler added)");
        if (hits > best)
        {
            best = hits;
            irq = cand[i];
        }
    }

    return (best >= MIN_IOHITS) ? irq : -1;
}
#endif

/*
 * The interrupt to test, with our handler in it where it fires.
 * Returns 0 when a precondition is broken (counted as INVALID).
 */
static int find_source(void)
{
    LONG n;

#if defined(__x86_64__)
    /* The PIT: arch/all-pc/timer/timer_init.c, KrnAddIRQHandler(0, TimerInt, ...) */
    src.s_IRQ     = 0;
    src.s_Kind    = SRC_TIMER;
    src.s_Movable = 1;
    src.s_Clock   = 1;
    src.s_Rate    = 50;
    src.s_Name    = "PIT";

    /* How often the clock can be looked at in a tick, for nap() */
    g_SpinPerTick = (ULONG)(spin_ticks(10, 0) / 10);
    if (g_SpinPerTick == 0)
        g_SpinPerTick = 1;

    n = go_live(WATCH_TICKS);
    if (n < MIN_HITS)
    {
        drop_live();
        bug("[smpirqaff] INVALID: IRQ %d %s\n", (int)src.s_IRQ,
            (n < 0) ? "takes no handler" : "does not fire");
        inval++;
        return 0;
    }
    live = 1;
#elif defined(__riscv)
    /*
     * The timer is not a source of the PLIC, a PCI host bridge's INTx
     * lines are: 32 to 35 on QEMU's virt machine, which is what the
     * bridge's interrupt-map says (arch/riscv64-sbi/hidd/pcidt).
     */
    static const UWORD cand[] = { 32, 33, 34, 35 };

    src.s_Kind    = SRC_IO;
    src.s_Movable = 1;
    src.s_Name    = "PCI INTx";
#if defined(HAVE_PLATFORMTIMER)
    {
        /* Not a PLIC source: every CPU has its own timer behind it */
        struct KrnPlatformTimer *kpt;

        kpt = (struct KrnPlatformTimer *)KrnGetSystemAttr(KATTR_PlatformTimer);
        if (kpt && kpt != (APTR)-1 && kpt->kpt_IRQ != (ULONG)-1)
            src.s_Private = (LONG)kpt->kpt_IRQ;
    }
#endif

    src.s_IRQ = probe_io(cand, 4);
    if (src.s_IRQ >= 0)
    {
        n = go_live(PROBE_TICKS);
        if (n >= MIN_IOHITS)
            live = 1;
        else
            drop_live();
    }
    if (!live)
    {
        /* The calls can still be checked, on a source that is there */
        src.s_Kind = SRC_NONE;
        if (src.s_IRQ < 0)
            src.s_IRQ = cand[0];
        if (irq_cpu((ULONG)src.s_IRQ) == NOCPU)
            src.s_IRQ = -1;
        bug("[smpirqaff] no PLIC source fires while files are read\n");
    }
#elif defined(__aarch64__)
    IPTR base = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);

    if (base == 0 || base == (IPTR)-1 || base == (IPTR)BCM2712_BASE)
        return 1;

    if (base == (IPTR)BCM2711_BASE)
    {
        static const UWORD cand[] = { BCM2711_SDIRQ };

        /*
         * Compare 3 of the system timer is nobody's (timer.device has
         * compare 1, arch/aarch64-raspi/timer/timer_init.c); the GIC
         * has the four of them from 96.
         */
        src.s_IRQ     = BCM2711_GPUIRQ + 3;
        src.s_Kind    = SRC_OWNED;
        src.s_Movable = 1;
        src.s_Rate    = ST_RATE;
        src.s_Private = GIC_TIMER_PPI;
        src.s_Name    = "system timer compare 3";

        n = go_live(WATCH_TICKS);
        if (n >= MIN_HITS)
        {
            live = 1;
            return 1;
        }
        drop_live();
        bug("[smpirqaff] IRQ %d does not fire on this machine\n", (int)src.s_IRQ);

        /* Then the SD host's, if reading files raises it */
        src.s_Kind = SRC_IO;
        src.s_Rate = 0;
        n = probe_io(cand, 1);
        if (n >= 0)
        {
            src.s_IRQ  = n;
            src.s_Name = "SD host";
            n = go_live(PROBE_TICKS);
            if (n >= MIN_IOHITS)
            {
                live = 1;
                return 1;
            }
            drop_live();
        }

        /* The calls can still be checked, on the timer's interrupt */
        src.s_IRQ  = BCM2711_GPUIRQ + 3;
        src.s_Kind = SRC_NONE;
        src.s_Name = "system timer compare 3";
        return 1;
    }

    /*
     * The others: system timer compare 1, timer.device's, on the legacy
     * controller. All its interrupts are the boot CPU's.
     */
    src.s_IRQ   = 1;
    src.s_Kind  = SRC_TIMER;
    src.s_Clock = 1;
    src.s_Name  = "system timer compare 1";

    g_SpinPerTick = (ULONG)(spin_ticks(10, 0) / 10);
    if (g_SpinPerTick == 0)
        g_SpinPerTick = 1;

    n = go_live(WATCH_TICKS);
    if (n >= MIN_HITS)
        live = 1;
    else
    {
        drop_live();
        src.s_Kind = SRC_NONE;
        bug("[smpirqaff] IRQ %d does not fire on this machine\n", (int)src.s_IRQ);
    }
#else
    (void)n;
#endif

    return 1;
}

static void part1(void)
{
    IPTR cpu = irq_cpu((ULONG)src.s_IRQ);

    if (cpu == NOCPU)
    {
        bug("[smpirqaff] part 1: *** FAIL *** KrnGetIRQA() reports no CPU for IRQ %d\n",
            (int)src.s_IRQ);
        fail++;
    }
    else if (cpu != 0)
    {
        bug("[smpirqaff] part 1: *** FAIL *** KrnGetIRQA() reports CPU %lu for IRQ %d "
            "before any change\n", (unsigned long)cpu, (int)src.s_IRQ);
        fail++;
    }
    else
        ok(1, "KrnGetIRQA() reports CPU 0 before any change");

    if (live)
        check_delivery(1, 0);
    else
        bug("[smpirqaff] part 1: SKIP delivery (no interrupt that fires)\n");
}

/* Back to the boot CPU, and is it there? */
static void back_to_boot(int part)
{
    IPTR got;

    if (!irq_set((ULONG)src.s_IRQ, (IPTR)TASKAFFINITY_BOOT))
    {
        bug("[smpirqaff] part %d: *** FAIL *** KrnModifyIRQA(IRQ %d, TASKAFFINITY_BOOT) "
            "returned FALSE\n", (int)part, (int)src.s_IRQ);
        fail++;
        return;
    }
    got = irq_cpu((ULONG)src.s_IRQ);
    if (got != 0)
    {
        bug("[smpirqaff] part %d: *** FAIL *** KrnGetIRQA() reports CPU %lu after "
            "TASKAFFINITY_BOOT\n", (int)part, (unsigned long)got);
        fail++;
        return;
    }
    ok(part, "TASKAFFINITY_BOOT accepted and KrnGetIRQA() reports CPU 0");

    if (live)
        check_delivery(part, 0);
}

/* Part 2 where only the boot CPU can be had (the Pi 3's controller) */
static void part2_boot_only(void)
{
    cpumask_t *two = KrnAllocCPUMask();
    IPTR aff[4], got;
    static const char * const name[4] =
    {
        "TASKAFFINITY_BOOT", "TASKAFFINITY_ANY", "a mask of CPU 0", "a mask of CPUs 0 and 1"
    };
    int i, bad = 0;

    bug("[smpirqaff] part 2: a mask of CPU 1 is refused: only the boot CPU can be had here\n");

    if (two)
    {
        KrnClearCPUMask(two);
        KrnGetCPUMask(0, two);
        KrnGetCPUMask(1, two);
    }
    aff[0] = (IPTR)TASKAFFINITY_BOOT;
    aff[1] = (IPTR)TASKAFFINITY_ANY;
    aff[2] = (IPTR)g_Mask[0];
    aff[3] = (IPTR)two;

    for (i = 0; i < (two ? 4 : 3); i++)
    {
        if (!irq_set((ULONG)src.s_IRQ, aff[i]))
        {
            bug("[smpirqaff] part 2: *** FAIL *** %s, which names the boot CPU, was refused\n",
                name[i]);
            fail++;
            bad = 1;
        }
        else if ((got = irq_cpu((ULONG)src.s_IRQ)) != 0)
        {
            bug("[smpirqaff] part 2: *** FAIL *** KrnGetIRQA() reports CPU %lu after %s\n",
                (unsigned long)got, name[i]);
            fail++;
            bad = 1;
        }
    }
    if (!bad)
        ok(2, "every request that names the boot CPU is accepted, and CPU 0 reported");

    if (irq_set((ULONG)src.s_IRQ, (IPTR)g_Mask[1]))
    {
        bug("[smpirqaff] part 2: *** FAIL *** a mask of CPU 1 was refused, then accepted\n");
        fail++;
    }
    else if ((got = irq_cpu((ULONG)src.s_IRQ)) != 0)
    {
        bug("[smpirqaff] part 2: *** FAIL *** KrnGetIRQA() reports CPU %lu after a refused "
            "request\n", (unsigned long)got);
        fail++;
    }
    else
        ok(2, "a mask of CPU 1 is refused, and CPU 0 still reported");

    if (two)
        KrnFreeCPUMask(two);

    if (live)
        check_delivery(2, 0);
}

/* Returns 1 when the interrupt can be given to another CPU */
static int part2(int ncpus)
{
    int set[3], nset = 0, i, n, moved = 0;
    IPTR got;

    set[nset++] = 1;
    if (ncpus > 4)
        set[nset++] = ncpus / 2;
    if (ncpus - 1 != 1)
        set[nset++] = ncpus - 1;

    for (i = 0; i < nset && !clock_dead; i++)
    {
        n = set[i];

        /* The reads are asked for on another CPU than the interrupt's */
        if (src.s_Kind == SRC_IO)
            pin_self((n + 1) % ncpus);

        if (!irq_set((ULONG)src.s_IRQ, (IPTR)g_Mask[n]))
        {
            if (i == 0 && !src.s_Movable &&
                irq_set((ULONG)src.s_IRQ, (IPTR)TASKAFFINITY_BOOT))
            {
                part2_boot_only();
                return 0;
            }
            bug("[smpirqaff] part 2: *** FAIL *** KrnModifyIRQA(IRQ %d, a mask of CPU %d) "
                "returned FALSE\n", (int)src.s_IRQ, (int)n);
            fail++;
            continue;
        }
        moved++;

        got = irq_cpu((ULONG)src.s_IRQ);
        if (got != (IPTR)n)
        {
            bug("[smpirqaff] part 2: *** FAIL *** IRQ %d given to CPU %d, KrnGetIRQA() "
                "reports %lu\n", (int)src.s_IRQ, (int)n, (unsigned long)got);
            fail++;
        }
        else
        {
            snprintf(what, sizeof(what), "IRQ %d given to CPU %d and reported there",
                     (int)src.s_IRQ, n);
            ok(2, what);
        }

        if (live)
            check_delivery(2, n);
    }

    if (src.s_Kind == SRC_IO)
        pin_self(-1);

    if (clock_dead)
        return 0;

    if (!live)
        bug("[smpirqaff] part 2: SKIP delivery (no interrupt that fires)\n");

    back_to_boot(2);

    return moved > 0;
}

static void read_loads(IPTR *load, int ncpus)
{
    int i;

    for (i = 0; i < ncpus; i++)
        load[i] = (IPTR)KrnGetCPUAttr(KATTR_CPULoad, i);
}

static void print_loads(const char *when, const IPTR *load, int ncpus)
{
    static char line[MAXCPUS * 12 + 4];
    int i, p = 0;

    line[0] = 0;
    for (i = 0; i < ncpus && p < (int)sizeof(line) - 12; i++)
        p += snprintf(line + p, sizeof(line) - p, " %lu", (unsigned long)load[i]);
    bug("[smpirqaff] part 3: KATTR_CPULoad %s, CPU 0 to %d:%s\n", when, (int)(ncpus - 1), line);
}

static int loads_same(const IPTR *load, int ncpus)
{
    int i;

    for (i = 1; i < ncpus; i++)
    {
        if (load[i] != load[0])
            return 0;
    }
    return 1;
}

/* Is 'cpu' less loaded than every other CPU of 'mask' (NULL: than every other)? */
static int load_lowest(const IPTR *load, int ncpus, int cpu, cpumask_t *mask)
{
    int i;

    for (i = 0; i < ncpus; i++)
    {
        if (i == cpu || (mask && !KrnCPUInMask(i, mask)))
            continue;
        if ((intptr_t)load[i] <= (intptr_t)load[cpu])
            return 0;
    }
    return 1;
}

/* One request with busy tasks running: did it take the idle CPU? */
static void least_loaded(int ncpus, int idle, IPTR affinity, cpumask_t *mask, const char *name)
{
    IPTR l1[MAXCPUS], l2[MAXCPUS], got;
    ULONG r;

    read_loads(l1, ncpus);
    r = irq_set((ULONG)src.s_IRQ, affinity);
    got = irq_cpu((ULONG)src.s_IRQ);
    read_loads(l2, ncpus);

    print_loads("before", l1, ncpus);
    print_loads("after", l2, ncpus);
    bug("[smpirqaff] part 3: %s: KrnModifyIRQA() %s, KrnGetIRQA() reports CPU %ld\n",
        name, r ? "TRUE" : "FALSE", (long)got);

    if (r && got == (IPTR)idle)
    {
        snprintf(what, sizeof(what), "%s chose the idle CPU, %d", name, idle);
        ok(3, what);
    }
    else if (loads_same(l1, ncpus) && loads_same(l2, ncpus))
    {
        bug("[smpirqaff] part 3: INVALID - KATTR_CPULoad reads the same for every CPU\n");
        inval++;
    }
    else if (!load_lowest(l1, ncpus, idle, mask) || !load_lowest(l2, ncpus, idle, mask))
    {
        bug("[smpirqaff] part 3: INVALID - by KATTR_CPULoad the idle CPU, %d, was not the "
            "least loaded\n", (int)idle);
        inval++;
    }
    else if (!r)
    {
        bug("[smpirqaff] part 3: *** FAIL *** KrnModifyIRQA(IRQ %d, %s) returned FALSE\n",
            (int)src.s_IRQ, name);
        fail++;
    }
    else
    {
        bug("[smpirqaff] part 3: *** FAIL *** %s chose CPU %ld, the least loaded is %d\n",
            name, (long)got, (int)idle);
        fail++;
    }
}

static void part3(int ncpus)
{
    int idle = ncpus - 1, other = idle - 1, i, created = 0, left;
    cpumask_t *two;

    memset(busy, 0, sizeof(busy));
    bug("[smpirqaff] part 3: this task is on CPU %d at priority %d\n",
        (int)KrnGetCPUNumber(), (int)FindTask(NULL)->tc_Node.ln_Pri);
    for (i = 0; i < idle; i++)
    {
        busy[i].b_Task = spawn("smpirqaff.busy", BusyWorker, i, &busy[i]);
        if (busy[i].b_Task && sleep_flag(&busy[i].b_Started, WAIT_TICKS))
            created++;
        else
            bug("[smpirqaff] part 3: the busy task for CPU %d did not start (task 0x%p, "
                "state %d, this is CPU %d at priority %d)\n", (int)i, busy[i].b_Task,
                busy[i].b_Task ? (int)busy[i].b_Task->tc_State : -1, (int)KrnGetCPUNumber(),
                (int)FindTask(NULL)->tc_Node.ln_Pri);
    }

    if (created != idle)
    {
        bug("[smpirqaff] part 3: INVALID - only %d of %d busy tasks started\n",
            (int)created, (int)idle);
        inval++;
    }
    else
    {
        bug("[smpirqaff] part 3: a busy task on CPUs 0 to %d, CPU %d idle\n",
            (int)(idle - 1), (int)idle);

        /* The interrupt is on the boot CPU: the clock is safe to wait for */
        Delay(LOAD_TICKS);

        least_loaded(ncpus, idle, (IPTR)TASKAFFINITY_ANY, NULL, "TASKAFFINITY_ANY");

        if (!irq_set((ULONG)src.s_IRQ, (IPTR)TASKAFFINITY_BOOT))
        {
            bug("[smpirqaff] part 3: *** FAIL *** KrnModifyIRQA(IRQ %d, TASKAFFINITY_BOOT) "
                "returned FALSE\n", (int)src.s_IRQ);
            fail++;
        }

        if ((two = KrnAllocCPUMask()) != NULL)
        {
            KrnClearCPUMask(two);
            KrnGetCPUMask(other, two);
            KrnGetCPUMask(idle, two);
            snprintf(label, sizeof(label), "a mask of CPUs %d (busy) and %d (idle)", other, idle);
            least_loaded(ncpus, idle, (IPTR)two, two, label);
            KrnFreeCPUMask(two);
        }
    }

    for (i = 0; i < idle; i++)
        busy[i].b_Stop = 1;
    for (i = 0, left = 0; i < idle; i++)
    {
        if (busy[i].b_Task && !sleep_flag(&busy[i].b_Done, WAIT_TICKS))
            left++;
    }
    if (left)
        bug("[smpirqaff] part 3: %d busy tasks did not end\n", (int)left);

    if (!irq_set((ULONG)src.s_IRQ, (IPTR)TASKAFFINITY_BOOT) || irq_cpu((ULONG)src.s_IRQ) != 0)
    {
        bug("[smpirqaff] part 3: *** FAIL *** IRQ %d did not go back to the boot CPU\n",
            (int)src.s_IRQ);
        fail++;
    }
}

/* A request that has to be refused, and to change nothing */
static void refused(ULONG irq, IPTR affinity, const char *name)
{
    IPTR before = irq_cpu(irq), after;
    ULONG r = irq_set(irq, affinity);

    after = irq_cpu(irq);

    if (r)
    {
        bug("[smpirqaff] part 4: *** FAIL *** %s: KrnModifyIRQA() returned TRUE\n", name);
        fail++;
    }
    else if (after != before)
    {
        bug("[smpirqaff] part 4: *** FAIL *** %s: refused, but KrnGetIRQA() reported %ld "
            "before and %ld after\n", name, (long)before, (long)after);
        fail++;
    }
    else
    {
        snprintf(what, sizeof(what), "%s is refused and changes nothing", name);
        ok(4, what);
    }
}

static void part4(int movable)
{
    cpumask_t *empty = KrnAllocCPUMask();
    ULONG irq = (ULONG)src.s_IRQ;
    IPTR got;

    /* Away from the boot CPU, where "nothing changed" can be told from "reset" */
    if (movable && !irq_set(irq, (IPTR)g_Mask[1]))
        bug("[smpirqaff] part 4: IRQ %d stays on the boot CPU\n", (int)irq);
    bug("[smpirqaff] part 4: IRQ %d is on CPU %ld\n", (int)irq, (long)irq_cpu(irq));

    if (empty)
    {
        KrnClearCPUMask(empty);
        refused(irq, (IPTR)empty, "an empty mask");
        KrnFreeCPUMask(empty);
    }
    else
    {
        bug("[smpirqaff] part 4: INVALID - no CPU mask\n");
        inval++;
    }

    refused(irq, (IPTR)TASKAFFINITY_ALL_BUT_SELF, "TASKAFFINITY_ALL_BUT_SELF");

    refused(BAD_IRQ, (IPTR)TASKAFFINITY_BOOT, "an interrupt that does not exist (boot CPU)");
    refused(BAD_IRQ, (IPTR)g_Mask[1], "an interrupt that does not exist (CPU 1)");
    if ((got = irq_cpu(BAD_IRQ)) != NOCPU)
    {
        bug("[smpirqaff] part 4: *** FAIL *** KrnGetIRQA() reports CPU %ld for an interrupt "
            "that does not exist\n", (long)got);
        fail++;
    }
    else
        ok(4, "KrnGetIRQA() leaves the tag alone for an interrupt that does not exist");

    if (movable && src.s_Private >= 0)
    {
        snprintf(label, sizeof(label), "IRQ %d, private to each CPU, to CPU 1",
                 (int)src.s_Private);
        refused((ULONG)src.s_Private, (IPTR)g_Mask[1], label);
    }
    else
        bug("[smpirqaff] part 4: SKIP private interrupt (none known on this port)\n");

    if (!irq_set(irq, (IPTR)TASKAFFINITY_BOOT) || irq_cpu(irq) != 0)
    {
        bug("[smpirqaff] part 4: *** FAIL *** IRQ %d did not go back to the boot CPU\n",
            (int)irq);
        fail++;
    }
}

static void part5(int ncpus)
{
    ULONG t0, t1, last, now, lastreads, hits, reads, elapsed, need;
    int s, stalled = 0, iostall = 0, iomax = 0;

    __atomic_store_n(&g_Overlap, 0, __ATOMIC_SEQ_CST);

    memset(&mover, 0, sizeof(mover));
    mover.m_IRQ   = (ULONG)src.s_IRQ;
    mover.m_A     = 0;
    mover.m_B     = ncpus - 1;
    mover.m_NCPUs = ncpus;
    mover.m_Task  = spawn("smpirqaff.mover", MoverWorker, 1, &mover);
    if (mover.m_Task == NULL || !wait_flag(&mover.m_Started, WAIT_TICKS))
    {
        mover.m_Stop = 1;
        bug("[smpirqaff] part 5: INVALID - the moving task did not start\n");
        inval++;
        return;
    }

    bug("[smpirqaff] part 5: IRQ %d moved between CPUs 0 and %d, then all %d, for %d seconds\n",
        (int)src.s_IRQ, (int)(ncpus - 1), (int)ncpus, (int)P5_SECS);

    t0 = now_ticks();
    hits = last = total_hits();
    reads = lastreads = g_Reads;
    for (s = 0; s < P5_SECS && !clock_dead; s++)
    {
        if (s == P5_ALL)
            mover.m_Phase = 1;

        stir(50);

        now = total_hits();
        if (now == last)
            stalled++;
        last = now;

        if (g_Reads == lastreads)
        {
            if (++iostall > iomax)
                iomax = iostall;
        }
        else
            iostall = 0;
        lastreads = g_Reads;
    }
    t1 = now_ticks();

    mover.m_Stop = 1;
    if (!wait_flag(&mover.m_Done, WAIT_TICKS))
    {
        bug("[smpirqaff] part 5: *** FAIL *** KrnModifyIRQA() did not return "
            "(after %lu calls)\n", (unsigned long)mover.m_Calls);
        fail++;
        hung = 1;
        return;
    }
    ok(5, "every KrnModifyIRQA() returned");

    hits    = total_hits() - hits;
    reads   = g_Reads - reads;
    elapsed = t1 - t0;

    bug("[smpirqaff] part 5: %lu calls: %lu moved the interrupt, %lu returned FALSE; "
        "%lu interrupts in %lu ticks\n",
        (unsigned long)mover.m_Calls, (unsigned long)mover.m_Moved,
        (unsigned long)mover.m_Refused, (unsigned long)hits, (unsigned long)elapsed);

    if (mover.m_Moved == 0)
    {
        bug("[smpirqaff] part 5: *** FAIL *** no KrnModifyIRQA() succeeded while the "
            "interrupt fired\n");
        fail++;
    }
    else
        ok(5, "the interrupt was moved while it fired");

    if (rd(&g_Overlap))
    {
        bug("[smpirqaff] part 5: *** FAIL *** the handler was entered on two CPUs at once "
            "(%lu times)\n", (unsigned long)rd(&g_Overlap));
        fail++;
    }
    else
        ok(5, "the handler was never entered on two CPUs at once");

    if (clock_dead)
    {
        bug("[smpirqaff] part 5: *** FAIL *** the clock stopped while IRQ %d was moved: "
            "the interrupt is not served any more\n", (int)src.s_IRQ);
        fail++;
        return;
    }

    if (src.s_Kind == SRC_IO)
    {
        bug("[smpirqaff] part 5: %lu reads completed\n", (unsigned long)reads);
        if (iomax >= 2 || reads == 0)
        {
            bug("[smpirqaff] part 5: *** FAIL *** the reads stopped completing while the "
                "interrupt was moved (%d seconds without one)\n", (int)iomax);
            fail++;
        }
        else if (hits == 0)
        {
            bug("[smpirqaff] part 5: INVALID - no interrupt while %lu reads completed\n",
                (unsigned long)reads);
            inval++;
        }
        else
            ok(5, "the reads kept completing and the interrupt kept being served");
    }
    else
    {
        if (stalled)
        {
            bug("[smpirqaff] part 5: *** FAIL *** the interrupt was not served for %d of "
                "the %d seconds\n", (int)stalled, (int)P5_SECS);
            fail++;
        }
        else
            ok(5, "the interrupt was served throughout");

        if (src.s_Rate)
        {
            /* Half of the least it makes in that time */
            need = (ULONG)(((UQUAD)src.s_Rate * elapsed) / 100);
            if (hits < need)
            {
                bug("[smpirqaff] part 5: *** FAIL *** interrupts were lost: %lu in %lu ticks, "
                    "at least %lu a second are raised\n",
                    (unsigned long)hits, (unsigned long)elapsed, (unsigned long)src.s_Rate);
                fail++;
            }
            else
                ok(5, "no large loss of interrupts");
        }
    }

    back_to_boot(5);
}

static void part6(int ncpus)
{
    struct Snap a, b;
    ULONG on, runs, wrong, reads;
    int n = ncpus - 1, t, quiet = 0;

    memset(&softint, 0, sizeof(softint));
    softint.is_Node.ln_Type = NT_INTERRUPT;
    softint.is_Node.ln_Name = "smpirqaff";
    softint.is_Node.ln_Pri  = 0;
    softint.is_Code         = (VOID_FUNC)SoftHandler;
    softint.is_Data         = NULL;

    if (src.s_Kind == SRC_IO)
        pin_self(0);

    if (!irq_set((ULONG)src.s_IRQ, (IPTR)g_Mask[n]) || irq_cpu((ULONG)src.s_IRQ) != (IPTR)n)
    {
        bug("[smpirqaff] part 6: *** FAIL *** IRQ %d could not be given to CPU %d\n",
            (int)src.s_IRQ, (int)n);
        fail++;
        pin_self(-1);
        irq_set((ULONG)src.s_IRQ, (IPTR)TASKAFFINITY_BOOT);
        return;
    }

    stir(SETTLE_TICKS);
    snap(&a);
    reads = g_Reads;
    __atomic_store_n(&g_SoftOn, 1, __ATOMIC_SEQ_CST);
    stir(P6_TICKS);
    __atomic_store_n(&g_SoftOn, 0, __ATOMIC_SEQ_CST);
    snap(&b);
    reads = g_Reads - reads;

    /* None queued and none running before the numbers are read */
    for (t = 0; t < WAIT_TICKS && !clock_dead; t++)
    {
        nap(1);
        if (softint.is_Node.ln_Type == NT_SOFTINT || rd(&g_SoftIn))
            quiet = 0;
        else if (++quiet >= IDLE_TICKS)
            break;
    }

    if (src.s_Kind == SRC_IO)
        pin_self(-1);

    on    = b.sn_Hits[n] - a.sn_Hits[n];
    runs  = rd(&g_SoftRuns);
    wrong = rd(&g_SoftWrong);

    bug("[smpirqaff] part 6: %lu interrupts on CPU %d, %lu soft interrupt runs\n",
        (unsigned long)on, (int)n, (unsigned long)runs);

    if (clock_dead)
    {
        bug("[smpirqaff] part 6: *** FAIL *** the clock stopped with IRQ %d on CPU %d\n",
            (int)src.s_IRQ, (int)n);
        fail++;
        return;
    }

    if (on == 0)
    {
        if (src.s_Kind == SRC_IO && reads)
        {
            bug("[smpirqaff] part 6: INVALID - no interrupt while %lu reads completed\n",
                (unsigned long)reads);
            inval++;
        }
        else
        {
            bug("[smpirqaff] part 6: *** FAIL *** no interrupt arrived on CPU %d\n", (int)n);
            fail++;
        }
    }
    else if (runs == 0)
    {
        bug("[smpirqaff] part 6: *** FAIL *** Cause() from the handler on CPU %d was never "
            "served\n", (int)n);
        fail++;
    }
    else if (wrong)
    {
        bug("[smpirqaff] part 6: *** FAIL *** soft interrupt ran on CPU %d "
            "(%lu of %lu runs were not on CPU 0)\n",
            (int)rd(&g_SoftLastCPU), (unsigned long)wrong, (unsigned long)runs);
        fail++;
    }
    else
    {
        snprintf(what, sizeof(what),
                 "every soft interrupt caused by the handler on CPU %d ran on CPU 0", n);
        ok(6, what);
    }

    if (!irq_set((ULONG)src.s_IRQ, (IPTR)TASKAFFINITY_BOOT) || irq_cpu((ULONG)src.s_IRQ) != 0)
    {
        bug("[smpirqaff] part 6: *** FAIL *** IRQ %d did not go back to the boot CPU\n",
            (int)src.s_IRQ);
        fail++;
    }
}

/*
 * The interrupt back on the boot CPU and our handler out of it. The
 * request may be refused while the CPU the interrupt is on has one to
 * serve, so it is made again.
 */
static void restore(void)
{
    int i;

    if (src.s_IRQ >= 0)
    {
        for (i = 0; i < BOOT_TRIES; i++)
        {
            if (irq_set((ULONG)src.s_IRQ, (IPTR)TASKAFFINITY_BOOT))
                break;
            nap(1);
        }
        if (i == BOOT_TRIES || irq_cpu((ULONG)src.s_IRQ) != 0)
        {
            bug("[smpirqaff] *** FAIL *** IRQ %d could not be put back on the boot CPU "
                "(KrnGetIRQA() reports %ld)\n",
                (int)src.s_IRQ, (long)irq_cpu((ULONG)src.s_IRQ));
            fail++;
        }
    }

    drop_live();
}

static void cleanup(int ncpus)
{
    int i;

    pin_self(-1);
    io_end();
    for (i = 0; i < ncpus; i++)
    {
        if (g_Mask[i])
            KrnFreeCPUMask(g_Mask[i]);
        g_Mask[i] = NULL;
    }
}

int main(void)
{
    int ncpus, i, movable = 0;

    KernelBase = OpenResource("kernel.resource");
    TaskResBase = OpenResource("task.resource");
    ncpus = KernelBase ? KrnGetCPUCount() : 1;

    bug("[smpirqaff] start: cpus=%d\n", (int)ncpus);
    if (ncpus < 2)
    {
        bug("[smpirqaff] only %d CPU online - needs SMP; SKIP\n", (int)ncpus);
        return RETURN_OK;
    }
    if (ncpus > MAXCPUS)
        ncpus = MAXCPUS;

    for (i = 0; i < ncpus; i++)
    {
        if ((g_Mask[i] = KrnAllocCPUMask()) == NULL)
            break;
        KrnClearCPUMask(g_Mask[i]);
        KrnGetCPUMask(i, g_Mask[i]);
    }
    if (i < ncpus)
    {
        cleanup(ncpus);
        bug("[smpirqaff] INVALID: no CPU masks\n");
        bug("[smpirqaff] DONE: 0 PASS, 0 FAIL, 1 INVALID\n");
        return RETURN_WARN;
    }

    io_fib = AllocDosObject(DOS_FIB, NULL);

    if (!find_source() || src.s_IRQ < 0)
    {
        if (src.s_IRQ < 0 && !inval)
            bug("[smpirqaff] SKIP (no interrupt on this machine that a program can find)\n");
        drop_live();
        cleanup(ncpus);
        verdict();
        return inval ? RETURN_WARN : RETURN_OK;
    }

    bug("[smpirqaff] IRQ %d (%s)%s\n", (int)src.s_IRQ, src.s_Name,
        live ? "" : ": does not fire, only the calls are checked");

    part1();

    if (!clock_dead)
        movable = part2(ncpus);

    if (clock_dead)
        bug("[smpirqaff] part 3: not run, the clock stopped\n");
    else if (!movable)
        bug("[smpirqaff] part 3: SKIP (the interrupt cannot be given to another CPU)\n");
    else
        part3(ncpus);

    if (!clock_dead)
        part4(movable);

    if (clock_dead)
        bug("[smpirqaff] part 5: not run, the clock stopped\n");
    else if (!movable)
        bug("[smpirqaff] part 5: SKIP (the interrupt cannot be given to another CPU)\n");
    else if (!live)
        bug("[smpirqaff] part 5: SKIP (no interrupt that fires)\n");
    else
        part5(ncpus);

    if (hung)
        bug("[smpirqaff] part 6: not run after a hang\n");
    else if (clock_dead)
        bug("[smpirqaff] part 6: not run, the clock stopped\n");
    else if (!movable)
        bug("[smpirqaff] part 6: SKIP (the interrupt cannot be given to another CPU)\n");
    else if (!live)
        bug("[smpirqaff] part 6: SKIP (no interrupt that fires)\n");
    else
        part6(ncpus);

    /*
     * After a hang the calls below may not come back, and with the
     * clock stopped nothing that waits does: the verdict comes first.
     */
    if (hung || clock_dead)
        verdict();

    restore();
    cleanup(ncpus);

    verdict();

    return fail ? RETURN_FAIL : (inval ? RETURN_WARN : RETURN_OK);
}
#else
int main(void)
{
    return RETURN_WARN;
}
#endif
