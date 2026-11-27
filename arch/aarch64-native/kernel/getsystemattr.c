/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: KrnGetSystemAttr() - AArch64.

    The generic rom/kernel version returns -1 for KATTR_PeripheralBase, so the
    BCM2708 SoC drivers (USB/SD/i2c/mbox) that derive their MMIO base from it
    (__arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase)) would access a
    bogus address and fail to probe their hardware. Provide the arm-native
    behaviour here.
*/

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "kernel_intern.h"
#include "kernel_fb.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#include "kernel_cpu.h"
#endif

#include <proto/kernel.h>

#if defined(__AROSEXEC_SMP__)
/*
 * A core's load: the share of the last window its idle task did NOT
 * consume. Fixed point, 0xffffffff == 100%, as on x86, and worked out as
 * there: by each core for itself, from its own tick, about once a second,
 * whether or not anybody asks. A caller gets the figure of the last full
 * window, so two callers, or two CPUs that are compared, are measured over
 * the same length of time and not since each was last asked about.
 *
 * Called by a core from its tick, with interrupts masked. Only that core
 * switches its idle task in and out, and it does so with interrupts
 * masked, so the task's state, its launch time (iet_private1) and its
 * busy time (iet_private2) are read here as one: another core could see
 * the state of the next slice with the times of the last. No lock.
 *
 * The busy time is 32-bit maths, for the counter's ~71 minute wrap; the
 * stamp is whole, so that 0 only ever means no window has begun.
 */
void core_CPULoadTick(void)
{
    unsigned int cpu = GetCPUNumber();
    struct aarch64_CPULoadData *cl;
    struct Task *idler;
    struct IntETask *iet;
    UQUAD now;
    ULONG idleNow, window, load = 0;

    if ((cpu >= AARCH64_MAXCPUS) || !__arm_arosintern.ARMI_GetTime)
        return;

    /* Pairs with the release store in Exec_ARMCreateIdleTask() */
    idler = __atomic_load_n(&aarch64_IdleTask[cpu], __ATOMIC_ACQUIRE);
    if (!idler || !(iet = GetIntETask(idler)))
        return;

    cl = &aarch64_CPULoad[cpu];
    now = __arm_arosintern.ARMI_GetTime();

    /* The window is still open */
    if (cl->cpl_LastStamp && ((now - cl->cpl_LastStamp) < CPULOAD_WINDOW))
        return;

    window = (ULONG)(now - cl->cpl_LastStamp);
    idleNow = (ULONG)iet->iet_private2;

    /* An idle core is never preempted, so its idle task can sit in TS_RUN
     * without committing a slice - fold the in-progress part in. */
    if (idler->tc_State == TS_RUN)
        idleNow += (ULONG)now - (ULONG)iet->iet_private1;

    if (cl->cpl_LastStamp && window)
    {
        ULONG idle = idleNow - (ULONG)cl->cpl_LastIdle;

        if (idle < window)
            load = 0xffffffff - (ULONG)(((UQUAD)idle << 32) / window);

        /* Other cores read it, whole */
        __atomic_store_n(&cl->cpl_LastLoad, load, __ATOMIC_RELAXED);
    }

    cl->cpl_LastIdle = idleNow;
    cl->cpl_LastStamp = now;
}

/* 0 until the core has ticked through its first window, and for a core
 * that never started */
static intptr_t krnCPULoad(unsigned int cpu)
{
    if (cpu >= AARCH64_MAXCPUS)
        return 0;

    return (intptr_t)__atomic_load_n(&aarch64_CPULoad[cpu].cpl_LastLoad, __ATOMIC_RELAXED);
}
#endif

AROS_LH1(intptr_t, KrnGetSystemAttr,
    AROS_LHA(uint32_t, id, D0),
    struct KernelBase *, KernelBase, 29, Kernel)
{
    AROS_LIBFUNC_INIT

#if defined(__AROSEXEC_SMP__)
    if ((id >= KATTR_CPULoad) && (id < KATTR_CPULoad_END))
        return krnCPULoad(id - KATTR_CPULoad);
#endif

    switch (id)
    {
    case KATTR_Architecture:
        return (intptr_t)"aarch64-raspi";

    case KATTR_PeripheralBase:
        return (intptr_t)__arm_arosintern.ARMI_PeripheralBase;

    case KATTR_AffinityMask:
        return (intptr_t)__arm_arosintern.ARMI_AffinityMask;

    case KATTR_FrameBuffer:
        return (intptr_t)krn_fb_base();

    case KATTR_FrameBufferWidth:
        return (intptr_t)krn_fb_width();

    case KATTR_FrameBufferHeight:
        return (intptr_t)krn_fb_height();

    case KATTR_FrameBufferDepth:
        return (intptr_t)krn_fb_depth();

    case KATTR_FrameBufferPitch:
        return (intptr_t)krn_fb_pitch();

    default:
        return -1;
    }

    AROS_LIBFUNC_EXIT
}
