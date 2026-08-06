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

#include <kernel_base.h>
#include <kernel_globals.h>
#include <kernel_intr.h>

#include "etask.h"

#include "kernel_sbi.h"
#include "kernel_intern.h"

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

void krnTimerInit(uint32_t timebase_hz, uint32_t tick_hz)
{
    tick_interval = timebase_hz / tick_hz;
    __timebase_freq = timebase_hz;
    last_load_time = krnReadTime();

    sbi_set_timer(krnReadTime() + tick_interval);
    csr_set(sie, SIE_STIE);
}

void krnTimerTick(void)
{
    uint64_t now = krnReadTime();

    __timer_ticks++;
    sbi_set_timer(now + tick_interval);

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
