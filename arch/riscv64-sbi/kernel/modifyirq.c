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
#include <kernel_irqaffinity.h>

#include <proto/kernel.h>
#if defined(__AROSEXEC_SMP__)
#include <proto/exec.h>
#endif

#define D(x)

/*
 * Controllers that collect interrupts onto one of the platform's
 * sources. There is one per bridge, and few enough of those that a
 * short table costs nothing.
 *
 * On SMP the table is covered by the lock that covers the handler
 * chains (kb_IntrSpinLock): KrnModifyIRQA() fills an entry in from a
 * task on any CPU while the interrupt path on another looks one up.
 */
struct krnMSIController __msi_ctrl[KRN_MAX_MSI_CONTROLLERS];

/*
 * The controller collecting onto a source, for the interrupt path. It
 * gets a copy, taken under the lock, so that it never works from an
 * entry that is half filled in and holds no lock while it runs the
 * handlers (krnRunIRQHandlers() takes the same one).
 */
int krnGetMSIController(struct KernelBase *KernelBase, unsigned int irq,
                        struct krnMSIController *mc)
{
    unsigned int i;
    int found = 0;

#if defined(__AROSEXEC_SMP__)
    KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_READ);
#else
    (void)KernelBase;
#endif
    for (i = 0; i < KRN_MAX_MSI_CONTROLLERS; i++)
    {
        if (__msi_ctrl[i].irq == irq && __msi_ctrl[i].status)
        {
            *mc = __msi_ctrl[i];
            found = 1;
            break;
        }
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
#endif

    return found;
}

/* The CPUs an interrupt can be given to: those with a PLIC context */
static int rv_IRQCPUUsable(struct KernelBase *KernelBase, unsigned int cpu)
{
    return krnPLICCPUUsable(cpu);
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

        Two things can be asked for on this platform, together or apart.

        Which CPU the source is delivered to. Every source of the
        interrupt controller goes to the boot CPU until this is asked
        for; the CPU is chosen once, when the call is made, and the
        source stays there until its affinity is set again.

        That the source is a message controller: given where its
        pending register is and which sources its vectors were given,
        servicing the source it raises fans out to those, so a driver
        is entered only for its own device.

    INPUTS
        irq     - The source to change.
        attribs - KERNELTAG_IRQ_AFFINITY: TASKAFFINITY_BOOT,
                  TASKAFFINITY_ANY or a CPU mask from KrnAllocCPUMask()
                  or KrnGetCPUMask(). Of the CPUs named, the least
                  loaded one is taken.
                  KERNELTAG_IRQ_MSISTATUS/MSIBASE/MSICOUNT: all three
                  describe a message controller.

    RESULT
        TRUE when everything asked for was done; FALSE, with nothing
        changed, when any part of it could not be.

    NOTES
        Only a source of the interrupt controller itself has a CPU of
        its own. The sources given to a message controller's vectors
        (KrnAllocIRQ()) are raised by the CPU that services the
        collecting source: their affinity cannot be set, and setting
        the collecting source's moves all of them.

        A CPU that is not running, or whose hart the firmware gave no
        context in the interrupt controller, cannot be given a source.

        The handlers of a source run on the CPU it is delivered to. A
        driver that asks for another CPU than the boot CPU has to keep
        its handler and its task code apart with more than Disable().

        Not to be called from an interrupt handler.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetIRQA(), KrnAllocIRQ()

    INTERNALS
        A source moved while the old CPU is servicing it is completed
        by that CPU first; see krnPLICSetTarget().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *irqTag, *affTag;
    IPTR status = 0, base = 0, count = 0;
    BOOL msi = FALSE;
    int cpu = -1, slot = -1;
    unsigned int given[RISCV_MAXCPUS];
    unsigned int i;
    ULONG changed = FALSE;

    if (!attribs)
        return FALSE;

    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_MSISTATUS, attribs)))
    {
        status = irqTag->ti_Data;
        msi = TRUE;
    }
    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_MSIBASE, attribs)))
    {
        base = irqTag->ti_Data;
        msi = TRUE;
    }
    if ((irqTag = LibFindTagItem(KERNELTAG_IRQ_MSICOUNT, attribs)))
    {
        count = irqTag->ti_Data;
        msi = TRUE;
    }
    affTag = LibFindTagItem(KERNELTAG_IRQ_AFFINITY, attribs);

    if (!msi && !affTag)
        return FALSE;
    if (msi && (!status || !base || !count))
        return FALSE;

    if (affTag)
    {
        /* Chosen here, once, before any lock: it asks about every CPU.
           Only the controller's own sources have a CPU to choose. */
        if (krnPLICGetTarget(irq) < 0)
            return FALSE;
        krnPLICCountTargets(given, RISCV_MAXCPUS);
        cpu = krnIRQAffinityCPUGiven(KernelBase, affTag->ti_Data, RISCV_MAXCPUS,
                                     rv_IRQCPUUsable, given);
        if (cpu < 0)
            return FALSE;
    }

#if defined(__AROSEXEC_SMP__)
    /*
     * Disable() only stops this CPU; another may be registering a
     * controller too, or looking this one up in krnGetMSIController().
     * The same lock keeps a source from being masked, unmasked or
     * throttled (intr.c) while it is given to another CPU.
     */
    Disable();
    KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
    if (msi)
    {
        for (i = 0; i < KRN_MAX_MSI_CONTROLLERS; i++)
        {
            if (!__msi_ctrl[i].status || __msi_ctrl[i].irq == irq)
            {
                slot = (int)i;
                break;
            }
        }
    }

    /* Nothing is changed unless all of it can be: the table has room
       before the source is moved, and the move is the last thing that
       can be refused */
    if ((!msi || slot >= 0) &&
        (!affTag || krnPLICSetTarget(irq, (unsigned int)cpu)))
    {
        if (msi)
        {
            __msi_ctrl[slot].irq    = irq;
            __msi_ctrl[slot].status = status;
            __msi_ctrl[slot].base   = (unsigned int)base;
            __msi_ctrl[slot].count  = (unsigned int)count;

            /*
             * Nothing adds handlers to the collecting source itself -
             * the drivers are on the sources its vectors were given -
             * so there is nothing else to unmask it, and until it is
             * the controller can receive messages that never arrive.
             */
            ictl_enable_irq(irq, KernelBase);

            D(bug("[KRN] %s: source %u collects %u vector(s) from %u\n",
                  __func__, irq, __msi_ctrl[slot].count, __msi_ctrl[slot].base);)
        }
        changed = TRUE;
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
    Enable();
#endif

    return changed;

    AROS_LIBFUNC_EXIT
}
