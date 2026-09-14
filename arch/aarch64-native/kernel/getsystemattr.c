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
 * The share of the window the idle task did NOT consume. 32-bit maths
 * throughout, for the counter's ~71 minute wrap; result is fixed point,
 * 0xffffffff == 100%, as on x86.
 */
static intptr_t krnCPULoad(unsigned int cpu)
{
    struct Task *idler;
    struct IntETask *iet;
    ULONG now, idleNow, window;
    intptr_t retval = 0;

    if ((cpu >= AARCH64_MAXCPUS) || !__arm_arosintern.ARMI_GetTime)
        return 0;

    idler = aarch64_IdleTask[cpu];
    if (!idler || !(iet = GetIntETask(idler)))
        return 0;

    now = (ULONG)__arm_arosintern.ARMI_GetTime();
    idleNow = (ULONG)iet->iet_private2;
    window = now - (ULONG)aarch64_CPULoad[cpu].cpl_LastStamp;

    /* Too soon to measure again - reuse the last answer. */
    if (aarch64_CPULoad[cpu].cpl_LastStamp && (window < CPULOAD_MINWINDOW))
        return (intptr_t)aarch64_CPULoad[cpu].cpl_LastLoad;

    /* An idle core is never preempted, so its idle task can sit in TS_RUN
     * without committing a slice - fold the in-progress part in. */
    if (idler->tc_State == TS_RUN)
        idleNow += now - (ULONG)iet->iet_private1;

    if (aarch64_CPULoad[cpu].cpl_LastStamp && window)
    {
        ULONG idle = idleNow - (ULONG)aarch64_CPULoad[cpu].cpl_LastIdle;

        if (idle >= window)
            retval = 0;
        else
            retval = (intptr_t)(0xffffffff - (ULONG)(((UQUAD)idle << 32) / window));
    }

    aarch64_CPULoad[cpu].cpl_LastIdle = idleNow;
    aarch64_CPULoad[cpu].cpl_LastStamp = now;
    aarch64_CPULoad[cpu].cpl_LastLoad = (ULONG)retval;

    return retval;
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
