/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Interrupt source attributes.
*/

#include <proto/arossupport.h>
#include <aros/kernel.h>

#include <resources/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_intern.h>
#include <kernel_interrupts.h>

#include <proto/kernel.h>

#define D(x)

/*
 * Controllers that collect interrupts onto one of the platform's
 * sources. There is one per bridge, and few enough of those that a
 * short table costs nothing.
 */
struct krnMSIController __msi_ctrl[KRN_MAX_MSI_CONTROLLERS];

struct krnMSIController *krnFindMSIController(unsigned int irq)
{
    unsigned int i;

    for (i = 0; i < KRN_MAX_MSI_CONTROLLERS; i++)
    {
        if (__msi_ctrl[i].irq == irq && __msi_ctrl[i].status)
            return &__msi_ctrl[i];
    }

    return NULL;
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

        The tags this platform understands describe a message
        controller: given where its pending register is and which
        sources its vectors were given, servicing the source it raises
        fans out to those, so a driver is entered only for its own
        device.

    INPUTS
        irq     - The source to change.
        attribs - KERNELTAG_IRQ_MSISTATUS/MSIBASE/MSICOUNT.

    RESULT
        TRUE when the source was changed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnAllocIRQ()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *irqTag;
    IPTR status = 0, base = 0, count = 0;
    unsigned int i;

    if (!attribs)
        return FALSE;

    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_MSISTATUS, attribs)))
        status = irqTag->ti_Data;
    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_MSIBASE, attribs)))
        base = irqTag->ti_Data;
    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_MSICOUNT, attribs)))
        count = irqTag->ti_Data;

    if (!status || !base || !count)
        return FALSE;

    for (i = 0; i < KRN_MAX_MSI_CONTROLLERS; i++)
    {
        if (!__msi_ctrl[i].status || __msi_ctrl[i].irq == irq)
        {
            __msi_ctrl[i].irq    = irq;
            __msi_ctrl[i].status = status;
            __msi_ctrl[i].base   = (unsigned int)base;
            __msi_ctrl[i].count  = (unsigned int)count;

            /*
             * Nothing adds handlers to the collecting source itself -
             * the drivers are on the sources its vectors were given -
             * so there is nothing else to unmask it, and until it is
             * the controller can receive messages that never arrive.
             */
            ictl_enable_irq(irq, KernelBase);

            D(bug("[KRN] %s: source %u collects %u vector(s) from %u\n",
                  __func__, irq, __msi_ctrl[i].count, __msi_ctrl[i].base);)
            return TRUE;
        }
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
}
