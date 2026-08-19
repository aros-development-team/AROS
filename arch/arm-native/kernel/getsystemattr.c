/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "kernel_intern.h"
#if defined(__AROSEXEC_SMP__)
#include "etask.h"
#include "kernel_cpu.h"
#endif

#include <proto/kernel.h>

#if defined(__AROSEXEC_SMP__)
/*
 * A core's load is whatever share of the window its idle task did NOT
 * consume. cpu_Switch already accumulates every task's busy microseconds
 * in iet_private2, so we only need the delta since our last sample.
 *
 * Derived on read rather than from a periodic sweep: the alternative
 * would be a walk of the scheduler lists from the CNTP FIQ handler, with
 * scheduler locks held and IPI delivery blocked for the duration.
 *
 * Everything is kept in 32 bits so it survives the microsecond counter's
 * ~71 minute wrap. Result is fixed point, 0xffffffff == 100%, matching
 * what x86 puts in cpu_Load.
 */
static intptr_t krnCPULoad(unsigned int cpu)
{
    struct Task *idler;
    struct IntETask *iet;
    ULONG now, idleNow, window;
    intptr_t retval = 0;

    if ((cpu >= ARM_MAXCPUS) || !__arm_arosintern.ARMI_GetTime)
        return 0;

    idler = arm_IdleTask[cpu];
    if (!idler || !(iet = GetIntETask(idler)))
        return 0;

    now = (ULONG)__arm_arosintern.ARMI_GetTime();
    idleNow = (ULONG)iet->iet_private2;
    window = now - (ULONG)arm_CPULoad[cpu].cpl_LastStamp;

    /* Too soon to measure again - reuse the last answer. */
    if (arm_CPULoad[cpu].cpl_LastStamp && (window < CPULOAD_MINWINDOW))
        return (intptr_t)arm_CPULoad[cpu].cpl_LastLoad;

    /*
     * An idle core is not preempted - nothing else is ready to run - so
     * its idle task can sit in TS_RUN for many windows without ever
     * committing its slice to iet_private2. Fold the in-progress part in
     * or a fully idle core reads as fully loaded.
     */
    if (idler->tc_State == TS_RUN)
        idleNow += now - (ULONG)iet->iet_private1;

    if (arm_CPULoad[cpu].cpl_LastStamp && window)
    {
        ULONG idle = idleNow - (ULONG)arm_CPULoad[cpu].cpl_LastIdle;

        if (idle >= window)
            retval = 0;
        else
            retval = (intptr_t)(0xffffffff - (ULONG)(((UQUAD)idle << 32) / window));
    }

    arm_CPULoad[cpu].cpl_LastIdle = idleNow;
    arm_CPULoad[cpu].cpl_LastStamp = now;
    arm_CPULoad[cpu].cpl_LastLoad = (ULONG)retval;

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
        return (intptr_t)"arm-raspi";

    case KATTR_PeripheralBase:
        return (intptr_t)__arm_arosintern.ARMI_PeripheralBase;

    case KATTR_AffinityMask:
        return (intptr_t)__arm_arosintern.ARMI_AffinityMask;

    default:
        return -1;
    }

    AROS_LIBFUNC_EXIT
}
