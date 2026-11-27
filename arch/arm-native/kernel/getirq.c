/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnGetIRQA() - ARM.
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
                  number of the CPU the interrupt is delivered to, which
                  on this platform is always the boot CPU.

    RESULT
        None. A tag is left as it was passed when the interrupt does not
        exist.

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

    /* Every interrupt of this controller is routed to the boot CPU */
    if ((irqTag = krnFindTagItem(KERNELTAG_IRQ_AFFINITY, attribs)))
        irqTag->ti_Data = 0;

    AROS_LIBFUNC_EXIT
}
