/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: pc KrnRemIRQHandler(): the generic one, entered in supervisor mode
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH1(void, KrnRemIRQHandler,

/*  SYNOPSIS */
        AROS_LHA(void *, handle, A0),

/*  LOCATION */
         struct KernelBase *, KernelBase, 8, Kernel)

/*  FUNCTION
        Remove a registered hardware IRQ handler.

    INPUTS
        handle - Handle returned by KrnAddIRQHandler().

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnAddIRQHandler(), KrnModifyIRQHandler()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntrNode *h = handle;

    if (h && (h->in_type == it_interrupt))
    {
        irqid_t irq = h->in_nr;
#if defined(__AROSEXEC_SMP__)
        APTR ssp = NULL;
#endif

        (void)goSuper();

        Disable();
#if defined(__AROSEXEC_SMP__)
        /*
         * Disabling the interrupt can need supervisor mode (the local APIC
         * controller clears IDT gates there), and it has to be done inside
         * the write lock, or an add on another CPU could enable the
         * interrupt first and have it disabled under its new handler.
         * Supervisor mode is entered with a syscall, and every syscall
         * takes this lock for reading in krnRunExceptionHandlers(): made
         * with the lock held for writing, as the generic version does, it
         * waits for itself. So enter first, and take the lock there.
         *
         * Interrupts stay off from Disable() to Enable(): the syscall gate
         * clears the flag, and UserState() returns with the flags saved
         * at the SuperState(), which Disable() had already cleared.
         */
        if (!KrnIsSuper())
            ssp = SuperState();
        KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
        REMOVE(h);
        if (IsListEmpty(&KERNELIRQ_LIST(irq)))
        {
                ictl_disable_irq(irq, KernelBase);
        }
#if defined(__AROSEXEC_SMP__)
        KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
        if (ssp)
            UserState(ssp);
#endif
        Enable();

        krnFreeIntrNode(h);

        goUser();
    }

    AROS_LIBFUNC_EXIT
}
