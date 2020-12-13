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

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH2(ULONG, KrnModifyIRQA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, irq, D0),
        AROS_LHA(struct TagItem *, attribs, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 39, Kernel)

/*  FUNCTION
        Modify an IRQ using the passed in tags.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE irqPol = (UBYTE)-1, irqTrig = (UBYTE)-1;
    struct TagItem *irqTag;

    D(bug("[Kernel] %s(%u, 0x%p)\n", __func__, irq, attribs);)

    if (attribs)
    {
        if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_POLARITY, attribs)))
        {    
            irqPol = (UBYTE)irqTag->ti_Data;
        }
        if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_TRIGGERLEVEL, attribs)))
        {    
            irqTrig = (UBYTE)irqTag->ti_Data;
        }
    }

    if ((irqPol != (UBYTE)-1) || (irqTrig != (UBYTE)-1))
    {
        struct IntrMapping *irqMap;
        BOOL newMp = FALSE;
        if ((irqMap = krnInterruptMapping(KernelBase, irq)) == NULL)
        {
            irqMap = AllocMem(sizeof(struct IntrMapping), MEMF_CLEAR);
            newMp = TRUE;
        }
        if (irqPol != (UBYTE)-1)
            irqMap->im_Polarity = irqPol;
        if (irqTrig != (UBYTE)-1)
            irqMap->im_Trig = irqTrig;
        D(bug("[Kernel] %s: pol = %u, trig = %u\n", __func__, irqMap->im_Polarity, irqMap->im_Trig);)
        if (newMp)
        {
            Enqueue(&KernelBase->kb_InterruptMappings, &irqMap->im_Node);
        }
    }
    /* If the interupt is already enabled, disable and re-enable to update its state .. */
    if (ictl_is_irq_enabled(irq, KernelBase))
    {
        ictl_disable_irq(irq,KernelBase);
        ictl_enable_irq(irq, KernelBase);
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
