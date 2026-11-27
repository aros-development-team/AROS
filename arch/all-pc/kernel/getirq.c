/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <proto/arossupport.h>
#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <resources/kernel.h>

#include <aros/debug.h>

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
        irq     - The interrupt to ask about.
        attribs - The ti_Data of each of these tags is replaced:
                  KERNELTAG_IRQ_POLARITY     - 0 = default, 1 = high, 2 = low.
                  KERNELTAG_IRQ_TRIGGERLEVEL - 0 = default, 1 = level, 2 = edge.
                  KERNELTAG_IRQ_AFFINITY     - the number of the CPU the
                      interrupt is delivered to. For an MSI vector, the CPU
                      its message is to be addressed to (KrnModifyIRQA()).

    RESULT
        None. A tag is left as it was passed when there is no answer: the
        interrupt does not exist or, for the polarity and trigger, nothing
        has been recorded for it.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnModifyIRQA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(%u, 0x%p)\n", __func__, irq, attribs);)

    if (attribs && (irq < HW_IRQ_COUNT))
    {
        struct IntrMapping *intrMap;
        struct TagItem *irqTag;
        /* The mappings list can be added to on another CPU (KrnModifyIRQA) */
        IPTR lockState = ictl_Lock();

        intrMap = krnInterruptMapping(KernelBase, irq);
        if (intrMap && (irqTag = LibFindTagItem(KERNELTAG_IRQ_POLARITY, attribs)))
        {
            irqTag->ti_Data = intrMap->im_Polarity;
        }
        if (intrMap && (irqTag = LibFindTagItem(KERNELTAG_IRQ_TRIGGERLEVEL, attribs)))
        {
            irqTag->ti_Data = intrMap->im_Trig;
        }
        if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_AFFINITY, attribs)))
        {
            /* An interrupt nothing has mapped is delivered to the boot CPU */
            irqTag->ti_Data = intrMap ? intrMap->im_CPU : 0;
        }
        ictl_Unlock(lockState);
    }

    AROS_LIBFUNC_EXIT
}
