/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.

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

#include "apic.h"

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
        Register a hardware IRQ handler.

    INPUTS
        irq          - Hardware-specific interrupt number.
        handler      - IRQ callback with the signature:
                       void IRQHandler(void *handlerData, void *handlerData2);
        handlerData  - First argument passed to the callback.
        handlerData2 - Second argument passed to the callback.

    RESULT
        Opaque handler handle, or NULL if registration fails.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        KrnModifyIRQHandler(), KrnRemIRQHandler()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct IntrNode *handle = NULL;

    D(bug("[KRN] %s(%02x, %012p, %012p, %012p):\n", __func__, irq, handler, handlerData, handlerData2));

    if (irq < HW_IRQ_COUNT)
    {
        /* Go to supervisor mode */
        (void)goSuper();

        handle = krnAllocIntrNode();
        D(bug("[KRN] %s: handle @ 0x%012p\n", __func__, handle));

        if (handle)
        {
            handle->in_Handler = handler;
            handle->in_HandlerData = handlerData;
            handle->in_HandlerData2 = handlerData2;
            handle->in_type = it_interrupt;
            handle->in_nr = irq;

            Disable();

            if (irq < HW_IRQ_COUNT)
            {
#if defined(__AROSEXEC_SMP__)
                /*
                 * Disable() only stops this CPU; another core may be walking
                 * this chain in krnRunIRQHandlers() right now.
                 *
                 * Unlike the generic version this lets go of the lock before
                 * the interrupt is enabled. Enabling can enter supervisor
                 * mode (the IDT is written there), that is a syscall, and
                 * every syscall takes this lock for reading on its way
                 * through krnRunExceptionHandlers(): held for writing here it
                 * would wait for itself. The controllers have their own lock
                 * (ictl_Lock()). The order still holds against a remove: that
                 * disables the interrupt inside its write lock and only when
                 * it left the chain empty, so either it sees this handler and
                 * disables nothing, or its disable comes before this enable.
                 */
                KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_WRITE);
#endif
                ADDHEAD(&KERNELIRQ_LIST(irq), &handle->in_Node);
#if defined(__AROSEXEC_SMP__)
                KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
#endif

                ictl_enable_irq(irq, KernelBase);
            }
            Enable();
        }

        goUser();
    }
    D(else bug("[KRN] %s: Invalid IRQ no #%u.", irq);)

    return handle;

    AROS_LIBFUNC_EXIT
}

/* Invoke registered IRQ callbacks with their stored arguments. */
void krnRunIRQHandlers(struct KernelBase *KernelBase, irqid_t irq)
{
    struct IntrNode *in, *in2, *in1;
    struct IntrNode **lastIntr = &in1;
    struct PlatformData *pdata;

    /*
     * The handler being run is recorded for KrnFmtAlertInfo(). The walk
     * itself uses locals: the record is per CPU only where there is per-CPU
     * data, and an iterator in a place two CPUs share would send both down
     * each other's chain.
     */
    if ((pdata = (struct PlatformData *)KernelBase->kb_PlatformData) != NULL)
    {
        struct CPUData *cpuData = core_APIC_GetCPUData(pdata->kb_APIC);

        if (cpuData)
            lastIntr = &cpuData->cpu_LastIntr;
        else
            lastIntr = &pdata->kb_LastIntr;
    }

#if defined(__AROSEXEC_SMP__)
    /*
     * Hold the chain in READ mode so a concurrent KrnAdd/RemIRQHandler()
     * on another CPU cannot mutate it mid-walk, or free the node being
     * run. Consequence: a handler must not add/remove IRQ handlers from
     * within itself - that would self-deadlock on the WRITE acquire.
     */
    KrnSpinLock(&KernelBase->kb_IntrSpinLock, NULL, SPINLOCK_MODE_READ);
#endif
    ForeachNodeSafe(&KERNELIRQ_LIST(irq), in, in2)
    {
        irqhandler_t h = in->in_Handler;

        *lastIntr = in;
        h(in->in_HandlerData, in->in_HandlerData2);
    }
#if defined(__AROSEXEC_SMP__)
    KrnSpinUnLock(&KernelBase->kb_IntrSpinLock);
#endif
    /* As before: what is left behind is the end of the chain, not a handler */
    *lastIntr = in;
}
