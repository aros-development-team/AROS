/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_cpu.h>
#include <kernel_debug.h>
#include <kernel_interrupts.h>
#include <kernel_objects.h>

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x)

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH4(void *, KrnAddIRQHandler,

/*  SYNOPSIS */
        AROS_LHA(uint32_t, irq, D0),
        AROS_LHA(irqhandler_t *, handler, A0),
        AROS_LHA(void *, handlerData, A1),
        AROS_LHA(void *, handlerData2, A2),

/*  LOCATION */
        struct KernelBase *, KernelBase, 7, Kernel)

/*  FUNCTION
        Add a raw hardware IRQ handler to the chain of handlers.

    INPUTS
        num          - hardware-specific IRQ number
        handler      - Pointer to a handler function
        handlerData,
        handlerData2 - User-defined data which is passed to the
                       handler.
        
          Handler function uses a C calling convention and must be
          declared as follows:

          void IRQHandler(void *handlerData, void *handlerData2)

          handlerData and handlerData2 will be values passed to the
          KrnAddExceptionHandler() function.

          There is no return code for the IRQ handler.

    RESULT
        An opaque handle that can be used for handler removal or NULL in case
        of failure (like unsupported exception number).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnRemIRQHandler()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct IntrNode *handle = NULL;

    D(bug("[KRN] KrnAddIRQHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));

    if (irq < HW_IRQ_COUNT)
    {
        /* Go to supervisor mode */
        (void)goSuper();

        handle = krnAllocIntrNode();
        D(bug("[KRN] handle=%012p\n", handle));

        if (handle)
        {
            handle->in_Handler = handler;
            handle->in_HandlerData = handlerData;
            handle->in_HandlerData2 = handlerData2;
            handle->in_type = it_interrupt;
            handle->in_nr = irq;

            Disable();
#if defined(__AROSEXEC_SMP__)
            /*
             * Disable() only stops this CPU; another core may be walking
             * this chain in krnRunIRQHandlers() right now.
             */
            KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
            ADDHEAD(&KERNELIRQ_LIST(irq), &handle->in_Node);

            ictl_enable_irq(irq, KernelBase);
#if defined(__AROSEXEC_SMP__)
            KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
#endif
            Enable();
        }

        goUser();
    }

    return handle;

    AROS_LIBFUNC_EXIT
}

/* Run IRQ handlers */
void krnRunIRQHandlers(struct KernelBase *KernelBase, irqid_t irq)
{
    struct IntrNode *in, *in2;

#if defined(__AROSEXEC_SMP__)
    /*
     * Hold the chain in READ mode so a concurrent KrnAdd/RemIRQHandler()
     * on another CPU cannot mutate it mid-walk. Consequence: a handler
     * must not add/remove IRQ handlers from within itself - that would
     * self-deadlock on the WRITE acquire.
     */
    KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_READ);
#endif
    ForeachNodeSafe(&KERNELIRQ_LIST(irq), in, in2)
    {
        irqhandler_t h = in->in_Handler;

        h(in->in_HandlerData, in->in_HandlerData2);
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
#endif
}
