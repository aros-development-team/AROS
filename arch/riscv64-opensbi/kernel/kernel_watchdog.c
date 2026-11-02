/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: SMP lock-up watchdog, served by the last CPU.

    Every hart leaves a trail in its TLS on each timer interrupt (what
    it interrupted) and while it waits for a spinlock. The last CPU to
    start - which otherwise runs little more than its idle task - reads
    the other harts' trails from its own timer interrupt, so it keeps
    working whatever they are doing, and reports on the serial console:

      - a hart that takes no timer interrupt for WD_STALL_SECS: it has
        interrupts off, or is stuck (there is no NMI to ask it more, so
        its trail is what it was doing just before);
      - a task that has run Forbid()den for WD_FORBID_SECS;
      - a spinlock wait longer than WD_SPIN_SECS.

    It also answers keys typed on the serial console (see wdHelp()).

    Built only with RISCV_KRNWATCHDOG (riscv-native/kernel/tls.h), and
    off unless "watchdog" is on the command line. Everything here is
    read without taking a lock - the hart being looked at may hold the
    lock forever - so list walks are bounded and may see a list in the
    middle of an update.
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)

#include <inttypes.h>

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/semaphores.h>
#include <stddef.h>
#include <proto/exec.h>

#define __KERNEL_NOLIBBASE__

#include <kernel_base.h>
#include <kernel_globals.h>
#include <kernel_interrupts.h>
#include <kernel_intr.h>

#include <proto/kernel.h>

#include <aros/types/spinlock_s.h>

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#define __AROS_KERNEL__
#include "exec_intern.h"
#include "etask.h"

#include "kernel_sbi.h"
#include "kernel_intern.h"

#if defined(RISCV_KRNWATCHDOG)

#define WD_STALL_SECS   2
#define WD_FORBID_SECS  5
#define WD_SPIN_SECS    1
#define WD_REPEAT_SECS  10      /* Summary line while a condition lasts */
#define WD_MAXNODES     256     /* Longest list walk */

extern tls_t __riscv_boot_tls;
int __krn_watchdog;
static BOOL wdInRAM(const void *p, IPTR len);
volatile uint32_t __krn_wdcpu;

/* What the watchdog last saw of each CPU */
static struct
{
    uint64_t        ticks;      /* WdTicks at the last check            */
    uint32_t        still;      /* Checks it has not moved for          */
    struct Task     *task;      /* Running task at the last check       */
    uint32_t        forbidden;  /* Checks it has run Forbid()den for    */
    uint8_t         stalled;    /* Reported, until it moves again       */
    uint8_t         forbidrep;
    uint8_t         spinrep;
    uint32_t        spinsecs;   /* Checks since the spin was reported   */
} wd[RISCV_MAXCPUS];

static uint64_t wd_ownticks;

/* ---- output --------------------------------------------------------- */

static void wdStr(const char *s)
{
    krnSBIPutStr(s);
}

static void wdHex(uint64_t v)
{
    krnSBIPutHex(v);            /* prints its own 0x */
}

static void wdDec(uint64_t v)
{
    krnSBIPutDec(v);
}

static void wdSDec(int64_t v)
{
    if (v < 0)
    {
        krnSBIPutC('-');
        v = -v;
    }
    krnSBIPutDec((uint64_t)v);
}

/* Elapsed timebase ticks as seconds with two decimals */
static void wdSecs(uint64_t dt)
{
    uint64_t f = __timebase_freq ? __timebase_freq : 1;
    uint64_t c = (dt % f) * 100 / f;

    krnSBIPutDec(dt / f);
    krnSBIPutC('.');
    krnSBIPutC('0' + (char)(c / 10));
    krnSBIPutC('0' + (char)(c % 10));
    krnSBIPutC('s');
}

static const char *wdName(struct Task *t)
{
    if (!t)
        return "<none>";
    return t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "<unnamed>";
}

static struct ExceptionContext *wdTaskCtx(struct Task *t)
{
    if (!t || !(t->tc_Flags & TF_ETASK) || !t->tc_UnionETask.tc_ETask)
        return NULL;

    return (struct ExceptionContext *)t->tc_UnionETask.tc_ETask->et_RegFrame;
}

static void wdTask(const char *tag, struct Task *t)
{
    struct ExceptionContext *c = wdTaskCtx(t);

    wdStr("[wd]   ");
    wdStr(tag);
    wdStr(" ");
    wdHex((IPTR)t);
    wdStr(" '");
    wdStr(wdName(t));
    wdStr("' pri ");
    wdSDec(t->tc_Node.ln_Pri);
    wdStr(" state ");
    wdDec(t->tc_State);
    wdStr(" wait ");
    krnSBIPutHex32(t->tc_SigWait);
    wdStr(" recvd ");
    krnSBIPutHex32(t->tc_SigRecvd);
    if (c)
    {
        wdStr(" pc ");
        wdHex(c->pc);
        wdStr(" ra ");
        wdHex(c->ra);
    }
    wdStr("\n");
}

static void wdBacktrace(uint64_t pc, uint64_t ra, uint64_t sp)
{
    APTR pcs[34];
    ULONG n = 0;

    if (!getKernelBase())
        return;

    if (pc)
        pcs[n++] = (APTR)pc;
    if (ra && ra != pc)
        pcs[n++] = (APTR)ra;
    if (sp)
        n += KrnBacktraceFromFrame((APTR)sp, &pcs[n], 32 - n);
    KrnPrintBacktrace("[wd] ", pcs, n);
}

static void wdTaskBacktrace(struct Task *t)
{
    struct ExceptionContext *c = wdTaskCtx(t);

    wdStr("[wd] --- '");
    wdStr(wdName(t));
    wdStr("'\n");
    if (c)
        wdBacktrace(c->pc, c->ra, c->sp);
    else
        wdStr("[wd]   (no saved context)\n");
}

/* ---- what a CPU is doing ------------------------------------------- */

/* The CPU's TLS, or NULL if the table entry does not look like one */
static tls_t *wdTLS(uint32_t cpu)
{
    tls_t *tls;

    if (cpu >= RISCV_MAXCPUS)
        return NULL;
    tls = __riscv_tls[cpu];
    /*
     * The boot hart's TLS is the kernel's own static __riscv_boot_tls: inside
     * the image, outside every MemHeader (Titan 2026-11-01: "TLS pointer
     * 0x802be0b8 is not a TLS" on every report, and CPU 0 - the one that
     * hangs - went unreported).
     */
    if (!tls || ((IPTR)tls & 7) ||
        (tls != &__riscv_boot_tls && !wdInRAM(tls, sizeof(tls_t))) || tls->CPUNumber != cpu)
    {
        if (tls)
        {
            wdStr("[wd] CPU ");
            wdDec(cpu);
            wdStr(": TLS pointer ");
            wdHex((IPTR)tls);
            wdStr(" is not a TLS - table clobbered?\n");
        }
        return NULL;
    }
    return tls;
}

static void wdLock(const char *name, spinlock_t *l)
{
    uint32_t w = l->lock;

    if (w == SPINLOCK_UNLOCKED)
        return;
    wdStr("[wd]   lock ");
    wdStr(name);
    wdStr(" ");
    wdHex((IPTR)l);
    wdStr(w & SPINLOCKF_WRITE ? " write-held" : " read-held");
    if (!(w & SPINLOCKF_WRITE))
    {
        wdStr(" x");
        wdDec(w & 0xffffff);
    }
    if ((w & SPINLOCKF_WRITE) && l->s_Owner)
    {
        wdStr(" by '");
        wdStr(wdName((struct Task *)l->s_Owner));
        wdStr("' ");
        wdHex((IPTR)l->s_Owner);
    }
    wdStr("\n");
}

/* exec's own locks that are held, and by whom */
static void wdExecLocks(void)
{
    struct IntExecBase *pe;

    if (!SysBase)
        return;
    pe = PrivExecBase(SysBase);

    wdStr("[wd] held exec locks:\n");
    wdLock("MemList", &pe->MemListSpinLock);
    wdLock("AllocMemList", &pe->AllocMemListSpinLock);
    wdLock("AllocatorCtxList", &pe->AllocatorCtxListSpinLock);
    wdLock("ResourceList", &pe->ResourceListSpinLock);
    wdLock("DeviceList", &pe->DeviceListSpinLock);
    wdLock("IntrList", &pe->IntrListSpinLock);
    wdLock("LibList", &pe->LibListSpinLock);
    wdLock("PortList", &pe->PortListSpinLock);
    wdLock("SemList", &pe->SemListSpinLock);
    wdLock("TaskRunning", &pe->TaskRunningSpinLock);
    wdLock("TaskSpinning", &pe->TaskSpinningLock);
    wdLock("TaskReady", &pe->TaskReadySpinLock);
    wdLock("TaskWait", &pe->TaskWaitSpinLock);
}

static void wdCPU(uint32_t cpu, uint64_t now, BOOL trace)
{
    tls_t *tls = wdTLS(cpu);
    uint64_t n, when;
    int i;

    if (!tls)
        return;

    n = tls->WdTicks;
    /* A hart that ticked after now was read is 0s behind, not -1 */
    when = tls->WdTime;
    if ((int64_t)(now - when) < 0)
        when = now;
    wdStr("[wd] CPU ");
    wdDec(cpu);
    wdStr(" (hart ");
    wdDec(tls->HartID);
    wdStr("): ");
    wdDec(n);
    wdStr(" ticks, last ");
    wdSecs(now - when);
    wdStr(" ago; IDNest ");
    wdSDec(tls->IDNestCnt);
    wdStr(" TDNest ");
    wdSDec(tls->TDNestCnt);
    wdStr(" trap depth ");
    wdSDec(tls->TrapDepth);
    wdStr("\n[wd]   running ");
    wdHex((IPTR)tls->ThisTask);
    wdStr(" '");
    wdStr(wdName(tls->ThisTask));
    wdStr("'\n");

    wdStr("[wd]   last tick: ");
    if (tls->WdTask)
    {
        wdStr("in '");
        wdStr(wdName(tls->WdTask));
        wdStr("' pc ");
        wdHex(tls->WdPC[(n - 1) & 7]);
        wdStr(" ra ");
        wdHex(tls->WdRA);
        wdStr(" sp ");
        wdHex(tls->WdSP);
        wdStr(" sstatus ");
        wdHex(tls->WdSR);
    }
    else
        wdStr("asleep in the dispatcher");
    wdStr("\n[wd]   recent pcs (oldest first):");
    for (i = 8; i > 0; i--)
    {
        if (n >= (uint64_t)i)
        {
            wdStr(" ");
            wdHex(tls->WdPC[(n - i) & 7]);
        }
    }
    wdStr("\n");

    if (tls->SpinWait)
    {
        spinlock_t *l = (spinlock_t *)tls->SpinWait;

        wdStr("[wd]   waiting ");
        wdSecs(now - tls->SpinSince);
        wdStr(" for spinlock ");
        wdHex((IPTR)l);
        wdStr(" (word ");
        krnSBIPutHex32(l->lock);
        wdStr(", owner '");
        wdStr(wdName((struct Task *)l->s_Owner));
        wdStr("' ");
        wdHex((IPTR)l->s_Owner);
        wdStr("), taken from ");
        wdHex((IPTR)tls->SpinCaller);
        wdStr("\n");
        if (trace && getKernelBase())
        {
            APTR pcs[1] = { tls->SpinCaller };

            KrnPrintBacktrace("[wd] caller ", pcs, 1);
        }
    }

    if (trace && tls->WdTask)
    {
        /* The stack from a little below the last tick's sp: frames the
           hart pushed since then are there too (as may be stale ones) */
        uint64_t sp = tls->WdSP;
        struct Task *t = tls->WdTask;

        if (sp > 2048 && (IPTR)t->tc_SPLower <= sp - 2048)
            sp -= 2048;
        else if ((IPTR)t->tc_SPLower < sp && (IPTR)t->tc_SPLower)
            sp = (IPTR)t->tc_SPLower;
        wdStr("[wd]   stack scan from ");
        wdHex(sp);
        wdStr(" (last tick's pc and ra first):\n");
        wdBacktrace(tls->WdPC[(n - 1) & 7], tls->WdRA, sp);
    }

    if (trace && tls->TrapDepth > 0 && tls->StackTop)
    {
        /* Inside a trap: its frames are on this hart's trap stack, not the
           task's. The live ones are nearest the top. */
        uint64_t top = (IPTR)tls->StackTop;

        wdStr("[wd]   in a trap (depth ");
        wdSDec(tls->TrapDepth);
        wdStr("): trap stack scan from ");
        wdHex(top - 4096);
        wdStr(":\n");
        wdBacktrace(0, 0, top - 4096);
    }
}

/* One line, for the CPUs a report is not about */
static void wdCPUShort(uint32_t cpu, uint64_t now)
{
    tls_t *tls = wdTLS(cpu);
    uint64_t when;

    if (!tls)
        return;
    when = tls->WdTime;
    if ((int64_t)(now - when) < 0)
        when = now;
    wdStr("[wd] CPU ");
    wdDec(cpu);
    wdStr(": tick ");
    wdSecs(now - when);
    wdStr(" ago, ID ");
    wdSDec(tls->IDNestCnt);
    wdStr(" TD ");
    wdSDec(tls->TDNestCnt);
    wdStr(" trap ");
    wdSDec(tls->TrapDepth);
    wdStr(" '");
    wdStr(wdName(tls->ThisTask));
    wdStr("'");
    if (tls->SpinWait)
    {
        wdStr(" spinning on ");
        wdHex((IPTR)tls->SpinWait);
    }
    wdStr("\n");
}

/*
 * The essentials on one line, repeated while the condition lasts: the
 * console path can drop characters in a long burst, and one clean copy
 * of this is what matters.
 */
static void wdSummary(uint32_t cpu, uint64_t now, const char *what)
{
    tls_t *tls = wdTLS(cpu);
    uint64_t n;

    if (!tls)
        return;
    n = tls->WdTicks;

    wdStr("[wd] STILL CPU ");
    wdDec(cpu);
    wdStr(" ");
    wdStr(what);
    wdStr(": '");
    wdStr(wdName(tls->ThisTask));
    wdStr("' ID ");
    wdSDec(tls->IDNestCnt);
    wdStr(" TD ");
    wdSDec(tls->TDNestCnt);
    wdStr(" trap ");
    wdSDec(tls->TrapDepth);
    wdStr(" pc ");
    wdHex(n ? tls->WdPC[(n - 1) & 7] : 0);
    if (tls->SpinWait)
    {
        spinlock_t *l = (spinlock_t *)tls->SpinWait;

        wdStr(" lock ");
        wdHex((IPTR)l);
        wdStr(" owner '");
        wdStr(wdName((struct Task *)l->s_Owner));
        wdStr("' from ");
        wdHex((IPTR)tls->SpinCaller);
        wdStr(" for ");
        wdSecs(now - tls->SpinSince);
    }
    wdStr("\n");
}

static void wdCPUs(uint64_t now, BOOL trace)
{
    uint32_t cpu;

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
        if (__riscv_tls[cpu])
            wdCPU(cpu, now, trace);
}

/* ---- tasks ---------------------------------------------------------- */

/*
 * Every list below belongs to exec and is walked without its lock: a node
 * can be removed and freed under the walker, and a freed node reads as the
 * mungwall fill (heavy4b-03: a5 = 0xdeadbeefdeadbeef on CPU 3). Each node
 * is therefore range-checked before its successor is read.
 */
static void wdList(const char *tag, struct List *l, BOOL trace)
{
    struct Node *n;
    int i = 0;

    for (n = l->lh_Head; n && wdInRAM(n, sizeof(struct Node)) && n->ln_Succ && i < WD_MAXNODES; n = n->ln_Succ, i++)
    {
        if (trace)
            wdTaskBacktrace((struct Task *)n);
        else
            wdTask(tag, (struct Task *)n);
    }
    if (i >= WD_MAXNODES)
        wdStr("[wd]   (list walk stopped)\n");
}

static void wdTasks(BOOL trace)
{
    struct IntExecBase *pe;
    uint32_t cpu;

    if (!SysBase)
        return;
    pe = PrivExecBase(SysBase);

    wdStr("[wd] running:\n");
    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
    {
        tls_t *tls = __riscv_tls[cpu];

        if (tls && tls->ThisTask)
        {
            if (trace)
                wdTaskBacktrace(tls->ThisTask);
            else
            {
                char tag[] = "CPUnn";

                tag[3] = '0' + (char)(cpu / 10);
                tag[4] = '0' + (char)(cpu % 10);
                wdTask(tag, tls->ThisTask);
            }
        }
    }
    wdStr("[wd] spinning:\n");
    wdList("s", &pe->TaskSpinning, trace);
    wdStr("[wd] ready:\n");
    wdList("r", &SysBase->TaskReady, trace);
    wdStr("[wd] waiting:\n");
    wdList("w", &SysBase->TaskWait, trace);
}

/* ---- semaphore waits ------------------------------------------------ */

/* RAM that exec manages: a pointer outside it is not followed */
static BOOL wdInRAM(const void *p, IPTR len)
{
    struct MemHeader *mh;
    int i = 0;

    if (!SysBase || ((IPTR)p & (sizeof(APTR) - 1)))
        return FALSE;
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh && mh->mh_Node.ln_Succ && i < 32;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ, i++)
    {
        if ((IPTR)p >= (IPTR)mh->mh_Lower && (IPTR)p + len <= (IPTR)mh->mh_Upper)
            return TRUE;
    }
    return FALSE;
}

/*
 * The semaphore a task is blocked on in ObtainSemaphore[Shared](), or NULL.
 * The waiter's SemaphoreRequest lives on its stack (semaphores.c) with
 * sr_Waiter = the task (bit 0 set for a shared request), linked into the
 * semaphore's ss_WaitQueue: find it near the saved sp, then follow the
 * list to its tail sentinel, which sits inside the SignalSemaphore.
 */
static struct SignalSemaphore *wdSemWait(struct Task *t, BOOL *shared)
{
    struct ExceptionContext *c;
    IPTR *sp, *top;
    int i;

    /* t comes from lists and owner fields other CPUs change under us */
    if (!wdInRAM(t, sizeof(struct Task)) || t->tc_Node.ln_Type > NT_PROCESS)
        return NULL;
    c = wdTaskCtx(t);
    if (!c || !wdInRAM(c, sizeof(*c)) || !(t->tc_SigWait & SIGF_SINGLE) ||
        !wdInRAM((APTR)c->sp, sizeof(IPTR)))
        return NULL;
    sp = (IPTR *)(c->sp & ~(IPTR)7);
    top = t->tc_SPUpper ? (IPTR *)t->tc_SPUpper : sp + 1024;

    for (i = 0; i < 1024 && sp + i + 1 < top; i++)
    {
        struct SemaphoreRequest *sr;
        struct MinNode *n;
        int hops;

        if ((sp[i] & ~(IPTR)SM_SHARED) != (IPTR)t)
            continue;
        sr = (struct SemaphoreRequest *)((UBYTE *)&sp[i] - offsetof(struct SemaphoreRequest, sr_Waiter));
        n = &sr->sr_Link;
        for (hops = 0; hops < 256; hops++)
        {
            if (!wdInRAM(n, sizeof(struct MinNode)))
                break;
            if (!n->mln_Succ)
            {
                /* n is &list->mlh_Tail */
                struct MinList *l = (struct MinList *)((UBYTE *)n - offsetof(struct MinList, mlh_Tail));
                struct SignalSemaphore *ss =
                    (struct SignalSemaphore *)((UBYTE *)l - offsetof(struct SignalSemaphore, ss_WaitQueue));

                if (wdInRAM(ss, sizeof(*ss)) && ss->ss_Link.ln_Type == NT_SIGNALSEM)
                {
                    if (shared)
                        *shared = (sp[i] & SM_SHARED) != 0;
                    return ss;
                }
                break;
            }
            n = n->mln_Succ;
        }
    }
    return NULL;
}

/* Name the library, device or resource whose base holds addr, if any */
static void wdWhere(const void *addr)
{
    struct List *lists[3];
    int l;

    if (!SysBase)
        return;
    lists[0] = &SysBase->LibList;
    lists[1] = &SysBase->DeviceList;
    lists[2] = &SysBase->ResourceList;
    for (l = 0; l < 3; l++)
    {
        struct Node *n;
        int i = 0;

        for (n = lists[l]->lh_Head; n && wdInRAM(n, sizeof(struct Node)) && n->ln_Succ && i < WD_MAXNODES; n = n->ln_Succ, i++)
        {
            IPTR lo = (IPTR)n, hi = (IPTR)n + 64;

            if (l < 2 || n->ln_Type == NT_LIBRARY || n->ln_Type == NT_DEVICE)
            {
                struct Library *lib = (struct Library *)n;

                lo = (IPTR)lib - lib->lib_NegSize;
                hi = (IPTR)lib + lib->lib_PosSize;
            }
            if ((IPTR)addr >= lo && (IPTR)addr < hi)
            {
                wdStr(" (in ");
                wdStr(n->ln_Name ? n->ln_Name : "?");
                wdStr(" base +");
                wdHex((IPTR)addr - (IPTR)n);
                wdStr(")");
                return;
            }
        }
    }
}

static void wdSemLine(struct Task *t, struct SignalSemaphore *ss, BOOL shared)
{
    if (!ss)
    {
        wdStr("[wd]   '");
        wdStr(wdName(t));
        wdStr("' (no longer waiting)\n");
        return;
    }
    wdStr("[wd]   '");
    wdStr(wdName(t));
    wdStr(shared ? "' wants shared " : "' wants ");
    wdHex((IPTR)ss);
    if (ss->ss_Link.ln_Name && wdInRAM(ss->ss_Link.ln_Name, 1))
    {
        wdStr(" \"");
        wdStr(ss->ss_Link.ln_Name);
        wdStr("\"");
    }
    wdWhere(ss);
    wdStr(" held by '");
    wdStr(ss->ss_Owner ? wdName(ss->ss_Owner) : "<shared>");
    wdStr("' ");
    wdHex((IPTR)ss->ss_Owner);
    wdStr(" nest ");
    wdSDec(ss->ss_NestCount);
    wdStr(" queue ");
    wdSDec(ss->ss_QueueCount);
    wdStr("\n");
}

/* Every task blocked on a semaphore, and who holds it */
static void wdSemaphores(void)
{
    struct Node *n;
    int i = 0, found = 0;

    if (!SysBase)
        return;
    wdStr("[wd] semaphore waits:\n");
    for (n = SysBase->TaskWait.lh_Head; n && wdInRAM(n, sizeof(struct Node)) && n->ln_Succ && i < WD_MAXNODES; n = n->ln_Succ, i++)
    {
        BOOL shared = FALSE;
        struct SignalSemaphore *ss = wdSemWait((struct Task *)n, &shared);

        if (ss)
        {
            wdSemLine((struct Task *)n, ss, shared);
            found++;
        }
    }
    if (!found)
        wdStr("[wd]   (none)\n");
}

/*
 * Follow waiter -> semaphore -> owner -> the semaphore that owner waits
 * for ... A walk that comes back to where it started is a deadlock. Each
 * cycle is reported once (by the lowest task address on it).
 */
static struct Task *wd_lastcycle;

static void wdSemDeadlocks(void)
{
    struct Node *n;
    int i = 0;

    if (!SysBase)
        return;
    for (n = SysBase->TaskWait.lh_Head; n && wdInRAM(n, sizeof(struct Node)) && n->ln_Succ && i < WD_MAXNODES; n = n->ln_Succ, i++)
    {
        struct Task *start = (struct Task *)n, *t = start, *lowest = start;
        int hops;

        for (hops = 0; hops < 8; hops++)
        {
            struct SignalSemaphore *ss = wdSemWait(t, NULL);

            if (!ss || !ss->ss_Owner || ss->ss_Owner == t)
                break;
            t = ss->ss_Owner;
            if (t < lowest)
                lowest = t;
            if (t == start)
            {
                if (lowest != wd_lastcycle)
                {
                    wd_lastcycle = lowest;
                    wdStr("\n[wd] *** semaphore deadlock ***\n");
                    /* The cycle can dissolve while it prints: every step
                       is re-read and the walk stops at the first gap */
                    t = start;
                    for (hops = 0; hops < 8 && t; hops++)
                    {
                        BOOL shared = FALSE;
                        struct SignalSemaphore *s2 = wdSemWait(t, &shared);

                        wdSemLine(t, s2, shared);
                        if (!s2)
                            break;
                        wdTaskBacktrace(t);
                        t = s2->ss_Owner;
                        if (t == start)
                            break;
                    }
                    wdStr("[wd] *** end of report ***\n");
                }
                return;
            }
        }
    }
}

/*
 * Every semaphore waiter with its backtrace, then every distinct owner
 * with its backtrace: the lock graph and where each party is, without
 * the cost of a backtrace of every task in the system ('B').
 */
static void wdSemWaitsTraced(void)
{
    struct Task *owners[16];
    struct Node *n;
    int nowners = 0, i = 0, j;

    if (!SysBase)
        return;
    wdStr("[wd] semaphore waits (traced):\n");
    for (n = SysBase->TaskWait.lh_Head; n && wdInRAM(n, sizeof(struct Node)) && n->ln_Succ && i < WD_MAXNODES; n = n->ln_Succ, i++)
    {
        BOOL shared = FALSE;
        struct SignalSemaphore *ss = wdSemWait((struct Task *)n, &shared);

        if (!ss)
            continue;
        wdSemLine((struct Task *)n, ss, shared);
        wdTaskBacktrace((struct Task *)n);
        if (ss->ss_Owner && nowners < 16)
        {
            for (j = 0; j < nowners; j++)
                if (owners[j] == ss->ss_Owner)
                    break;
            if (j == nowners)
                owners[nowners++] = ss->ss_Owner;
        }
    }
    wdStr("[wd] owners:\n");
    for (j = 0; j < nowners; j++)
    {
        struct Task *o = owners[j];
        uint32_t cpu;
        BOOL running = FALSE;

        if (!wdInRAM(o, sizeof(struct Task)))
            continue;
        for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
            if (__riscv_tls[cpu] && __riscv_tls[cpu]->ThisTask == o)
            {
                running = TRUE;
                wdStr("[wd] --- '");
                wdStr(wdName(o));
                wdStr("' is running on CPU ");
                wdDec(cpu);
                wdStr("\n");
                wdCPU(cpu, krnReadTime(), TRUE);
            }
        if (!running)
            wdTaskBacktrace(o);
    }
}

/* ---- the keys ------------------------------------------------------- */

static void wdHelp(void)
{
    wdStr("[wd] watchdog on CPU ");
    wdDec(__krn_wdcpu);
    wdStr(": c CPUs, C CPUs + stack scans, t tasks, B task backtraces,"
          " S semaphore waits, W waits+owners traced, l held exec locks, i irq counts, P PLIC state, ? this\n");
}

static void wdKey(int ch, uint64_t now)
{
    switch (ch)
    {
    case 'c':
        wdCPUs(now, FALSE);
        break;

    case 'C':
        wdCPUs(now, TRUE);
        break;

    case 't':
        wdTasks(FALSE);
        break;

    case 'B':
        wdTasks(TRUE);
        break;

    case 'l':
        wdExecLocks();
        break;

    case 'S':
        wdSemaphores();
        break;

    case 'W':
        wdSemWaitsTraced();
        break;

    case 'P':                   /* PLIC state of every source seen so far */
    {
        unsigned int n;

        for (n = 1; n < KRN_MAX_IRQ_SOURCES; n++)
            if (__irq_counts[n])
                krnPLICDumpState(n);
        break;
    }

    case 'i':
    {
        unsigned int n;

        wdStr("[wd] irqs:");
        for (n = 1; n < KRN_MAX_IRQ_SOURCES; n++)
        {
            if (__irq_counts[n])
            {
                wdStr(" ");
                wdDec(n);
                wdStr("=");
                wdDec(__irq_counts[n]);
            }
        }
        wdStr("\n");
        break;
    }

    case '\r':
    case '\n':
        break;

    default:
        wdHelp();
        break;
    }
}

/* ---- the checks ----------------------------------------------------- */

static void wdReport(uint32_t cpu, uint64_t now, const char *what)
{
    wdStr("\n[wd] *** CPU ");
    wdDec(cpu);
    wdStr(": ");
    wdStr(what);
    wdStr(" ***\n");
    wdCPU(cpu, now, TRUE);
    wdStr("[wd] the other CPUs:\n");
    {
        uint32_t i;

        for (i = 0; i < RISCV_MAXCPUS; i++)
            if (i != cpu && __riscv_tls[i])
                wdCPUShort(i, now);
    }
    wdExecLocks();
    wdTasks(FALSE);
    wdStr("[wd] *** end of report ***\n");
}

static void wdCheck(uint32_t self, uint64_t now)
{
    uint32_t cpu;

    for (cpu = 0; cpu < RISCV_MAXCPUS; cpu++)
    {
        tls_t *tls = __riscv_tls[cpu];
        uint64_t ticks;
        struct Task *t;

        if (!tls || cpu == self)
            continue;

        /* Timer interrupts */
        ticks = __atomic_load_n(&tls->WdTicks, __ATOMIC_ACQUIRE);
        if (ticks != wd[cpu].ticks)
        {
            if (wd[cpu].stalled)
            {
                wdStr("[wd] CPU ");
                wdDec(cpu);
                wdStr(" is taking timer interrupts again\n");
            }
            wd[cpu].ticks = ticks;
            wd[cpu].still = 0;
            wd[cpu].stalled = 0;
        }
        else if (ticks && ++wd[cpu].still >= WD_STALL_SECS)
        {
            if (!wd[cpu].stalled)
            {
                wd[cpu].stalled = 1;
                wdReport(cpu, now, "no timer interrupt for 2s - interrupts off, or stuck");
            }
            else if ((wd[cpu].still % WD_REPEAT_SECS) == 0)
                wdSummary(cpu, now, "takes no timer interrupts");
        }

        /* A task running Forbid()den (the idle tasks run at -128) */
        t = tls->ThisTask;
        if (t && t == wd[cpu].task && tls->TDNestCnt >= 0 &&
            t->tc_Node.ln_Pri != -128)
        {
            if (++wd[cpu].forbidden >= WD_FORBID_SECS)
            {
                if (!wd[cpu].forbidrep)
                {
                    wd[cpu].forbidrep = 1;
                    wdReport(cpu, now, "one task has run Forbid()den for 5s");
                }
                else if ((wd[cpu].forbidden % WD_REPEAT_SECS) == 0)
                    wdSummary(cpu, now, "runs one task Forbid()den");
            }
        }
        else
        {
            wd[cpu].forbidden = 0;
            wd[cpu].forbidrep = 0;
        }
        wd[cpu].task = t;

        /* A long spinlock wait */
        if (tls->SpinWait &&
            (now - tls->SpinSince) >= (uint64_t)WD_SPIN_SECS * __timebase_freq)
        {
            if (!wd[cpu].spinrep)
            {
                wd[cpu].spinrep = 1;
                wd[cpu].spinsecs = 0;
                wdReport(cpu, now, "waiting for a spinlock for over 1s");
            }
            else if ((++wd[cpu].spinsecs % WD_REPEAT_SECS) == 0)
                wdSummary(cpu, now, "waits for a spinlock");
        }
        else
            wd[cpu].spinrep = 0;
    }
}

/* ---- entry points --------------------------------------------------- */

/* Every hart, from its timer interrupt (ctx NULL: woken in the dispatcher) */
void krnWatchRecord(struct ExceptionContext *ctx)
{
    tls_t *tls = TLS_PTR_GET();
    uint64_t n = tls->WdTicks;

    tls->WdTime = krnReadTime();
    if (ctx)
    {
        tls->WdPC[n & 7] = ctx->pc;
        tls->WdRA = ctx->ra;
        tls->WdSP = ctx->sp;
        tls->WdSR = ctx->sr;
        tls->WdTask = tls->ThisTask;
    }
    else
    {
        tls->WdPC[n & 7] = 0;
        tls->WdTask = NULL;
    }
    __atomic_store_n(&tls->WdTicks, n + 1, __ATOMIC_RELEASE);
}

/* The watchdog CPU's timer interrupt, after its own tick work */
void krnWatchdogService(uint32_t self)
{
    uint64_t now = krnReadTime();
    int ch;

    wd_ownticks++;

    if ((ch = krnSBIGetC()) > 0)
        wdKey(ch, now);

    /* Once a second; quiet for the first five while the system starts */
    if ((wd_ownticks % 100) == 0 && wd_ownticks >= 500)
    {
        if (wd_ownticks == 500)
            wdHelp();
        wdCheck(self, now);
        if ((wd_ownticks % 500) == 0)
            wdSemDeadlocks();
    }
}

#endif /* RISCV_KRNWATCHDOG */
#endif /* __AROSEXEC_SMP__ */
