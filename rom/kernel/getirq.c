/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

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
        irq     - Interrupt identifier to query.
        attribs - Tag list naming the attributes wanted. The ti_Data of
                  each tag that is understood is replaced with the value.

    TAGS
        KERNELTAG_IRQ_AFFINITY (IPTR) - the number of the CPU the
            interrupt is delivered to.
        KERNELTAG_IRQ_POLARITY, KERNELTAG_IRQ_TRIGGERLEVEL - how the
            interrupt line signals, where the controller knows.

    RESULT
        None.

    NOTES
        A tag that is not understood, or an interrupt that is not known,
        leaves ti_Data as it was: set it to a value the attribute cannot
        have before the call to tell.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnModifyIRQA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return;

    AROS_LIBFUNC_EXIT
}
