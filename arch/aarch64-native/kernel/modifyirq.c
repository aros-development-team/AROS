/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnModifyIRQA() - AArch64.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <resources/kernel.h>

#include <kernel_base.h>
#include <kernel_irqaffinity.h>

#include "kernel_intern.h"
#include "kernel_cpu.h"

#include <proto/kernel.h>

/* A core that never came up is counted out of ARMI_AffinityMask */
static int a64_IRQCPUUsable(struct KernelBase *KernelBase, unsigned int cpu)
{
    return (__arm_arosintern.ARMI_AffinityMask & (1U << cpu)) != 0;
}

/*****************************************************************************

    NAME */

        AROS_LH2(ULONG, KrnModifyIRQA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, irq, D0),
        AROS_LHA(struct TagItem *, attribs, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 39, Kernel)

/*  FUNCTION
        Change how an interrupt source is treated.

        The tag this platform understands is KERNELTAG_IRQ_AFFINITY: the
        CPUs the interrupt may be delivered to. One of them is chosen,
        once, and the interrupt is delivered there from then on, whether
        or not it has a handler yet.

    INPUTS
        irq     - The source to change.
        attribs - KERNELTAG_IRQ_AFFINITY with TASKAFFINITY_BOOT,
                  TASKAFFINITY_ANY or a CPU mask.

    RESULT
        TRUE when the interrupt is now delivered to a CPU that was asked
        for. FALSE, with nothing changed, when it is not: the request
        names no running CPU, the interrupt is private to each CPU (the
        IPIs and per-CPU timers of a GIC), or the interrupt controller
        can not deliver it there. The controller of the Pi 3 routes all
        its interrupts together, so only the boot CPU can be had there:
        a request that names it is TRUE, any other is FALSE.

    NOTES
        A handler moved off the boot CPU can run at the same time as the
        task code of its driver, which Disable() no longer keeps apart.

        Must not be called from an interrupt handler.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetIRQA(), KrnAllocCPUMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *irqTag;
    ULONG changed = FALSE;
    int cpu;

    if (!attribs || (irq >= HW_IRQ_COUNT))
        return FALSE;

    if (!(irqTag = krnFindTagItem(KERNELTAG_IRQ_AFFINITY, attribs)))
        return FALSE;

    /*
     * No routing on this platform: everything is on the boot CPU already,
     * and it is the only CPU there is to choose. The request stands if it
     * names that CPU, whatever the load on the others.
     */
    if (!__arm_arosintern.ARMI_IRQSetTarget)
    {
        IPTR affinity = irqTag->ti_Data;

        if ((affinity == TASKAFFINITY_BOOT) || (affinity == TASKAFFINITY_ANY))
            return TRUE;
        if (affinity == TASKAFFINITY_ALL_BUT_SELF)
            return FALSE;
        return KrnCPUInMask(0, (void *)affinity) ? TRUE : FALSE;
    }

    /* The interrupts each core has been given, for a choice among equals */
    {
        unsigned int given[AARCH64_MAXCPUS], n;
        int at;

        for (n = 0; n < AARCH64_MAXCPUS; n++)
            given[n] = 0;
        if (__arm_arosintern.ARMI_IRQGetTarget)
        {
            for (n = 0; n < HW_IRQ_COUNT; n++)
            {
                at = __arm_arosintern.ARMI_IRQGetTarget(n);
                if ((at > 0) && (at < AARCH64_MAXCPUS))
                    given[at]++;
            }
        }

        cpu = krnIRQAffinityCPUGiven(KernelBase, irqTag->ti_Data, AARCH64_MAXCPUS,
                                     a64_IRQCPUUsable, given);
    }
    if (cpu < 0)
        return FALSE;

    /*
     * The lock that covers the handler chains. KrnAddIRQHandler() holds
     * it around the enable, which routes the interrupt, and every CPU
     * holds it in read mode while it runs handlers: with it held here no
     * handler is running anywhere. A CPU that has acknowledged the
     * interrupt and not taken the lock yet is not held off by this; it
     * looks at the target once it has the lock, and leaves the handlers
     * to the new CPU (gic400_run_irq()).
     */
    Disable();
#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
    if (__arm_arosintern.ARMI_IRQSetTarget(irq, cpu))
        changed = TRUE;
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
#endif
    Enable();

    return changed;

    AROS_LIBFUNC_EXIT
}
