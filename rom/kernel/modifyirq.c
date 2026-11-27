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

        AROS_LH2(ULONG, KrnModifyIRQA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, irq, D0),
        AROS_LHA(struct TagItem *, attribs, A0),

/*  LOCATION */
        struct KernelBase *, KernelBase, 39, Kernel)

/*  FUNCTION
        Configure platform-specific attributes of an interrupt source.

    INPUTS
        irq     - Interrupt identifier to configure.
        attribs - Tag list of platform-specific interrupt attributes.

    TAGS
        KERNELTAG_IRQ_AFFINITY (IPTR) - the CPUs the interrupt may be
            delivered to: TASKAFFINITY_BOOT, TASKAFFINITY_ANY or a CPU
            mask from KrnAllocCPUMask(). The interrupt is delivered to
            one CPU: of those named, the one with the lowest load at the
            time of the call. It stays there until its affinity is set
            again.
        KERNELTAG_IRQ_POLARITY, KERNELTAG_IRQ_TRIGGERLEVEL - how the
            interrupt line signals, where the controller can be told.

    RESULT
        TRUE if the request was carried out, FALSE if it was not.

    NOTES
        Every device interrupt is delivered to the boot CPU unless its
        affinity has been set. A handler written for one CPU excludes its
        interrupt with Disable(), and that only reaches the CPU it is
        called on: an interrupt should be moved by the driver that owns
        it, and that driver must share its data with the handler under a
        spinlock.

        The request fails if no CPU named can take the interrupt, and on
        hardware that routes all its interrupts together for any CPU but
        the boot CPU.

        Soft interrupts caused by the handler are not moved with it; they
        run on the boot CPU.

    EXAMPLE

    BUGS

    SEE ALSO
        KrnGetIRQA(), KrnAllocCPUMask(), KrnGetCPUMask()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* The implementation of this function is architecture-specific */
    return 0;

    AROS_LIBFUNC_EXIT
}
