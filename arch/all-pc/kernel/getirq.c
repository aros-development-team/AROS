/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <proto/arossupport.h>
#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <resources/kernel.h>

#include <aros/debug.h>

AROS_LH2(void, KrnGetIRQA,
    AROS_LHA(ULONG, irq, D0),
    AROS_LHA(struct TagItem *, attribs, A0),
    struct KernelBase *, KernelBase, 63, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] %s(%u, 0x%p)\n", __func__, irq, attribs);)

    if (attribs)
    {
        struct IntrMapping *intrMap = krnInterruptMapping(KernelBase, irq);
        struct TagItem *irqTag;

        if (intrMap && (irqTag = LibFindTagItem(KERNELTAG_IRQ_POLARITY, attribs)))
        {    
            irqTag->ti_Data = intrMap->im_Polarity;
        }
        if (intrMap && (irqTag = LibFindTagItem(KERNELTAG_IRQ_TRIGGERLEVEL, attribs)))
        {    
            irqTag->ti_Data = intrMap->im_Trig;
        }
    }


    AROS_LIBFUNC_EXIT
}
