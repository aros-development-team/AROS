/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnGetIRQA() - AArch64.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <resources/kernel.h>

#include <kernel_base.h>

#include "kernel_intern.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(void, KrnGetIRQA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, irq, D0),
        AROS_LHA(struct TagItem *, attribs, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 63, Kernel)

/*  FUNCTION
        Query an IRQ using the passed in tags.

    INPUTS
        irq     - The source to ask about.
        attribs - KERNELTAG_IRQ_AFFINITY: its ti_Data is replaced with the
                  number of the CPU the interrupt is delivered to.

    RESULT
        None. A tag is left as it was passed when there is no answer: the
        interrupt does not exist, or it is private to each CPU (the IPIs
        and per-CPU timers of a GIC) and so has no one CPU.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnModifyIRQA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *irqTag;

    if (!attribs || (irq >= HW_IRQ_COUNT))
        return;

    if ((irqTag = krnFindTagItem(KERNELTAG_IRQ_AFFINITY, attribs)))
    {
        /* No routing on this platform: everything is on the boot CPU */
        int cpu = 0;

        /* One byte, written whole by KrnModifyIRQA(): no lock to read it */
        if (__arm_arosintern.ARMI_IRQGetTarget)
            cpu = __arm_arosintern.ARMI_IRQGetTarget(irq);

        if (cpu >= 0)
            irqTag->ti_Data = cpu;
    }

    AROS_LIBFUNC_EXIT
}
