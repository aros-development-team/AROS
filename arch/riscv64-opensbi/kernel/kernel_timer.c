/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: SBI timer tick for the opensbi-riscv64 target.

    Programs a periodic S-mode timer interrupt through the SBI TIME
    extension. The tick counter is the future scheduler heartbeat; for
    now it just counts.
*/

#include <inttypes.h>

#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <asm/cpu.h>

#define __KERNEL_NOLIBBASE__

#include <kernel_base.h>
#include <kernel_globals.h>
#include <kernel_intr.h>

#include <proto/kernel.h>

#include "etask.h"

#include "kernel_sbi.h"
#include "kernel_intern.h"

/*
 * Answering a keypress on the serial console costs a read of the console
 * on every tick, and pulls the task lists and backtrace printing in with
 * it. Off unless asked for.
 */
//#define KRNDEBUG_POKE

volatile uint64_t __timer_ticks;

/*
 * CPU load accounting, after the model the x86 APIC heartbeat uses:
 * the dispatcher accumulates the time spent in wfi into
 * __cpu_sleeptime, and once a second of timebase the load becomes the
 * busy fraction of the elapsed window, scaled to 0..0xffffffff.
 */
volatile uint64_t __cpu_sleeptime;
volatile uint32_t __cpu_load;
uint64_t __timebase_freq;

static uint64_t tick_interval;
static uint64_t last_load_time;

/*
 * Close a task's accounting window: its share of the elapsed second,
 * scaled to 0..0xffffffff (the same scale the per-CPU load uses).
 * cpu_Switch() accumulated the task's run segments into iet_private2.
 */
static void krnTaskUsage(struct Task *t, uint64_t window)
{
    struct IntETask *iet;

    if (!(t->tc_Flags & TF_ETASK) || !t->tc_UnionETask.tc_ETask)
        return;

    iet = IntETask(t->tc_UnionETask.tc_ETask);
    if (iet->iet_private2 >= window)
        iet->iet_CpuUsage = 0xffffffff;
    else
        iet->iet_CpuUsage = (ULONG)((iet->iet_private2 << 32) / window);
    iet->iet_private2 = 0;
}

#if defined(KRNDEBUG_POKE)

static struct ExceptionContext *dbgTaskCtx(struct Task *t)
{
    if (!t || !(t->tc_Flags & TF_ETASK) || !t->tc_UnionETask.tc_ETask)
        return NULL;

    return (struct ExceptionContext *)t->tc_UnionETask.tc_ETask->et_RegFrame;
}

static APTR dbgTaskPC(struct Task *t)
{
    struct ExceptionContext *c = dbgTaskCtx(t);

    return c ? (APTR)c->pc : NULL;
}

static void dbgPokeTask(struct Task *t, char tag)
{
    LONG pri = t->tc_Node.ln_Pri;

    krnSBIPutStr("[dbg]  ");
    krnSBIPutC(tag);
    krnSBIPutStr(" '");
    krnSBIPutStr(t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "<unnamed>");
    krnSBIPutStr("' pri ");
    if (pri < 0)
    {
        krnSBIPutC('-');
        pri = -pri;
    }
    krnSBIPutDec(pri);
    if ((t->tc_Flags & TF_ETASK) && t->tc_UnionETask.tc_ETask)
    {
        ULONG u = IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuUsage;

        krnSBIPutStr(" cpu ");
        krnSBIPutDec(((u >> 16) * 1000) >> 16);
        krnSBIPutStr("/1000 sig 0x");
        krnSBIPutHex32(t->tc_SigWait);
    }
    krnSBIPutStr(" pc 0x");
    krnSBIPutHex((uint64_t)(IPTR)dbgTaskPC(t));
    krnSBIPutStr("\n");
}

/*
 * Backtrace a task that is not the one we interrupted.
 *
 * A task gives up the CPU through cpu_Switch(), which leaves its whole
 * register frame in the ETask, so a waiting task can be unwound exactly
 * like the running one. Without this the only task that can ever be
 * backtraced is whichever one the timer happened to interrupt - on an
 * idle machine, the idle loop - which says nothing about the task that
 * is actually stuck.
 */
static void dbgPokeBacktrace(struct Task *t)
{
    struct ExceptionContext *c = dbgTaskCtx(t);
    APTR pcs[34];
    ULONG n = 0;

    krnSBIPutStr("[dbg] --- '");
    krnSBIPutStr(t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "<unnamed>");
    krnSBIPutStr("'\n");

    if (!c)
    {
        krnSBIPutStr("[dbg] (no saved context)\n");
        return;
    }

    pcs[n++] = (APTR)c->pc;
    if (c->x[0] && c->x[0] != c->pc)
        pcs[n++] = (APTR)c->x[0];
    n += KrnBacktraceFromFrame((APTR)c->x[1], &pcs[n], 32 - n);
    KrnPrintBacktrace("[dbg] ", pcs, n);
}

/*
 * A keypress on the serial console proves the kernel is alive and shows
 * what the machine is doing - the question a frozen VNC screen cannot
 * answer. Serves from the timer tick, so it works whenever interrupts do.
 */
static void krnDebugPoke(int ch, struct ExceptionContext *ctx)
{
    struct Task *t;

    switch (ch)
    {
    case 't':                   /* every task, by list */
        if (!SysBase)
            break;
        if (SysBase->ThisTask)
            dbgPokeTask(SysBase->ThisTask, 'R');
        ForeachNode(&SysBase->TaskReady, t)
            dbgPokeTask(t, 'r');
        ForeachNode(&SysBase->TaskWait, t)
            dbgPokeTask(t, 'w');
        break;

    case 'B':                   /* where every task is */
        if (!SysBase)
            break;
        if (SysBase->ThisTask)
            dbgPokeBacktrace(SysBase->ThisTask);
        ForeachNode(&SysBase->TaskReady, t)
            dbgPokeBacktrace(t);
        ForeachNode(&SysBase->TaskWait, t)
            dbgPokeBacktrace(t);
        break;

    case 'i':                   /* interrupt delivery counts */
    {
        unsigned int n;

        krnSBIPutStr("[dbg] ticks ");
        krnSBIPutDec(__timer_ticks);
        krnSBIPutStr(", irqs:");
        for (n = 1; n < KRN_MAX_IRQ_SOURCES; n++)
        {
            if (__irq_counts[n])
            {
                krnSBIPutStr(" ");
                krnSBIPutDec(n);
                krnSBIPutStr("=");
                krnSBIPutDec(__irq_counts[n]);
            }
        }
        krnSBIPutStr("\n");
        break;
    }

    case 'b':                   /* where the interrupted task is */
        if (getKernelBase() && ctx)
        {
            APTR pcs[34];
            ULONG n = 0;

            pcs[n++] = (APTR)ctx->pc;
            if (ctx->x[0] && ctx->x[0] != ctx->pc)
                pcs[n++] = (APTR)ctx->x[0];
            n += KrnBacktraceFromFrame((APTR)ctx->x[1], &pcs[n], 32 - n);
            KrnPrintBacktrace("[dbg] ", pcs, n);
        }
        else
            krnSBIPutStr("[dbg] (idle)\n");
        break;

    default:                    /* one-line liveness status */
        krnSBIPutStr("[dbg] up ");
        krnSBIPutDec(__timer_ticks / 100);
        krnSBIPutStr("s load ");
        krnSBIPutDec(((__cpu_load >> 16) * 100) >> 16);
        krnSBIPutStr("% task '");
        krnSBIPutStr((SysBase && SysBase->ThisTask &&
                      SysBase->ThisTask->tc_Node.ln_Name)
                     ? SysBase->ThisTask->tc_Node.ln_Name : "<none>");
        krnSBIPutStr("' ('t' tasks, 'b' backtrace, 'B' all blocked, 'i' irqs)\n");
        break;
    }
}

#endif /* KRNDEBUG_POKE */

void krnTimerInit(uint32_t timebase_hz, uint32_t tick_hz)
{
    tick_interval = timebase_hz / tick_hz;
    __timebase_freq = timebase_hz;
    last_load_time = krnReadTime();

    sbi_set_timer(krnReadTime() + tick_interval);
    csr_set(sie, SIE_STIE);
}

void krnTimerTick(struct ExceptionContext *ctx)
{
    uint64_t now = krnReadTime();

    __timer_ticks++;
    sbi_set_timer(now + tick_interval);

#if defined(KRNDEBUG_POKE)
    {
        /* Serial debug poke - see krnDebugPoke() */
        int ch;

        if ((ch = krnSBIGetC()) > 0)
            krnDebugPoke(ch, ctx);
    }
#else
    (void)ctx;
#endif

    if (__timebase_freq && (now - last_load_time) >= __timebase_freq)
    {
        uint64_t window = now - last_load_time;
        uint64_t slept = __cpu_sleeptime;
        struct KernelBase *kbase = getKernelBase();

        if (slept > window)
            slept = window;
        __cpu_load = (uint32_t)(((window - slept) << 32) / window);
        __cpu_sleeptime = 0;
        last_load_time = now;

        /* Record the result in the boot hart's entry; the accumulator
           moves into HartData when the other harts start running */
        if (kbase && kbase->kb_PlatformData)
        {
            struct HartData *hd =
                &kbase->kb_PlatformData->kb_Harts[kbase->kb_PlatformData->kb_BootHart];

            hd->hd_Load = __cpu_load;
            hd->hd_SleepTime = slept;
            hd->hd_LastLoadTime = now;
        }

        /* And every task's share of the window */
        if (SysBase)
        {
            struct Task *t;

            ForeachNode(&SysBase->TaskReady, t)
                krnTaskUsage(t, window);
            ForeachNode(&SysBase->TaskWait, t)
                krnTaskUsage(t, window);

            t = SysBase->ThisTask;
            if (t && t->tc_State == TS_RUN && (t->tc_Flags & TF_ETASK) &&
                t->tc_UnionETask.tc_ETask)
            {
                /* Fold in the segment it is still running */
                struct IntETask *iet = IntETask(t->tc_UnionETask.tc_ETask);

                iet->iet_private2 += now - iet->iet_private1;
                iet->iet_private1 = now;
                krnTaskUsage(t, window);
            }
        }
    }

    /*
     * Drive exec's periodic interrupt. timer.device hangs its request
     * queues off this, so without it nothing that waits on a timeout
     * ever wakes - the tick alone only serves the scheduler, which is
     * driven straight from the trap handler.
     */
    if (SysBase)
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);
}
