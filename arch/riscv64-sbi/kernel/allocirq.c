/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Interrupt source allocation.
*/

#include <aros/kernel.h>
#include <aros/irqtypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_intern.h>

#include <proto/kernel.h>
#if defined(__AROSEXEC_SMP__)
#include <proto/exec.h>
#endif

#define D(x)

/* One byte per source number the handler lists have; those below the
   controller's own count are never taken from here */
UBYTE __virq_used[HW_IRQ_COUNT];

/*
 * Sources the PLIC does not drive.
 *
 * The handler lists are HW_IRQ_COUNT long while the interrupt
 * controller only ever claims what the tree gave it, so the numbers
 * above that are free for things that have no wire: a bridge that
 * receives message interrupts takes one per vector and raises it with
 * KrnRunIRQ() when its status register says that vector arrived.
 *
 * Nothing here excludes a second caller: KrnAllocIRQ() and KrnFreeIRQ()
 * hold kb_IntrSpinLock around these on SMP, and the kernel's own early
 * call (kernel_timer.c) is made while only the boot hart runs.
 */
ULONG krnAllocVirtualIRQ(ULONG count)
{
    ULONG base = krnPLICSourceCount() + 1;
    ULONG irq, n;

    if (!count || (base + count) > HW_IRQ_COUNT)
        return (ULONG)-1;

    for (irq = base; (irq + count) <= HW_IRQ_COUNT; irq++)
    {
        for (n = 0; n < count; n++)
        {
            if (__virq_used[(irq + n) - base])
                break;
        }

        if (n == count)
        {
            for (n = 0; n < count; n++)
                __virq_used[(irq + n) - base] = 1;

            D(bug("[KRN] %s: %u source(s) from %u\n", __func__, count, irq);)
            return irq;
        }
    }

    return (ULONG)-1;
}

void krnFreeVirtualIRQ(ULONG start, ULONG count)
{
    ULONG base = krnPLICSourceCount() + 1;
    ULONG n;

    if (start < base)
        return;

    for (n = 0; n < count; n++)
    {
        if ((start + n) < HW_IRQ_COUNT)
            __virq_used[(start + n) - base] = 0;
    }
}

/*****************************************************************************

    NAME */

        AROS_LH2(ULONG, KrnAllocIRQ,

/*  SYNOPSIS */
        AROS_LHA(ULONG, irq_type, D0),
        AROS_LHA(ULONG, count, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 38, Kernel)

/*  FUNCTION
        Allocate interrupt sources of a given kind.

    INPUTS
        irq_type - IRQTYPE_MSI is the only kind this platform has to
                   give: numbers with no wire behind them, for a bus
                   driver that tells its own sources apart.
        count    - How many consecutive sources are wanted.

    RESULT
        The first source number, or -1.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnFreeIRQ(), KrnRunIRQ()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (irq_type == IRQTYPE_MSI)
    {
        ULONG irq;

#if defined(__AROSEXEC_SMP__)
        /* Tasks on two CPUs must not be given the same numbers */
        Disable();
        KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
        irq = krnAllocVirtualIRQ(count);
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
        Enable();
#endif
        return irq;
    }

    return (ULONG)-1;

    AROS_LIBFUNC_EXIT
}
