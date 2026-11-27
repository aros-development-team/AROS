/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnModifyIRQA() - ARM.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <resources/kernel.h>

#include <exec/tasks.h>

#include <kernel_base.h>

#include "kernel_intern.h"

#include <proto/kernel.h>

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
        CPUs the interrupt may be delivered to. The interrupt controller
        routes all its interrupts together and they are all delivered to
        the boot CPU, so that is the only CPU that can be had.

    INPUTS
        irq     - The source to change.
        attribs - KERNELTAG_IRQ_AFFINITY with TASKAFFINITY_BOOT,
                  TASKAFFINITY_ANY or a CPU mask.

    RESULT
        TRUE when the request names the boot CPU, which the interrupt is
        delivered to already. FALSE otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetIRQA(), KrnAllocCPUMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *irqTag;
    IPTR affinity;

    if (!attribs || (irq >= HW_IRQ_COUNT))
        return FALSE;

    if (!(irqTag = krnFindTagItem(KERNELTAG_IRQ_AFFINITY, attribs)))
        return FALSE;

    /*
     * The boot CPU is the only CPU there is to choose: the request stands
     * if it names that CPU, whatever the load on the others.
     */
    affinity = irqTag->ti_Data;

    if ((affinity == TASKAFFINITY_BOOT) || (affinity == TASKAFFINITY_ANY))
        return TRUE;
    if (affinity == TASKAFFINITY_ALL_BUT_SELF)
        return FALSE;
    return KrnCPUInMask(0, (void *)affinity) ? TRUE : FALSE;

    AROS_LIBFUNC_EXIT
}
