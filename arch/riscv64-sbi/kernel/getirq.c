/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Interrupt source attributes, read back.
*/

#include <proto/arossupport.h>
#include <aros/kernel.h>

#include <resources/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_intern.h>

#include <proto/kernel.h>
#if defined(__AROSEXEC_SMP__)
#include <proto/exec.h>
#endif

/*****************************************************************************

    NAME */

        AROS_LH2(void, KrnGetIRQA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, irq, D0),
        AROS_LHA(struct TagItem *, attribs, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 63, Kernel)

/*  FUNCTION
        Query an interrupt source using the passed in tags.

    INPUTS
        irq     - The source to ask about.
        attribs - KERNELTAG_IRQ_AFFINITY: the tag's ti_Data is replaced
                  with the number of the CPU the source is delivered to.

    RESULT
        None; the answers are written into the tags.

    NOTES
        A source given to a vector of a message controller is answered
        with the CPU of the source that controller collects onto, which
        is the CPU its handlers run on. For any other source that is
        not the interrupt controller's own the tag is left as it was.

        Not to be called from an interrupt handler.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnModifyIRQA()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *irqTag;

    if (attribs && (irqTag = LibFindTagItem(KERNELTAG_IRQ_AFFINITY, attribs)))
    {
        unsigned int i;
        int cpu;

#if defined(__AROSEXEC_SMP__)
        /* The controller table and the targets change under this lock
           (KrnModifyIRQA) */
        Disable();
        KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_READ);
#endif
        cpu = krnPLICGetTarget(irq);
        for (i = 0; (cpu < 0) && (i < KRN_MAX_MSI_CONTROLLERS); i++)
        {
            if (__msi_ctrl[i].status && (irq >= __msi_ctrl[i].base) &&
                ((irq - __msi_ctrl[i].base) < __msi_ctrl[i].count))
                cpu = krnPLICGetTarget(__msi_ctrl[i].irq);
        }
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
        Enable();
#endif

        if (cpu >= 0)
            irqTag->ti_Data = (IPTR)cpu;
    }

    AROS_LIBFUNC_EXIT
}
