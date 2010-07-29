#include <aros/libcall.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_cpu.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "kernel_memory.h"

/* We use own implementation of bug(), so we don't need aros/debug.h */
#define D(x)

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH4(void *, KrnAddIRQHandler,

/*  SYNOPSIS */
        AROS_LHA(uint8_t, irq, D0),
        AROS_LHA(void *, handler, A0),
        AROS_LHA(void *, handlerData, A1),
        AROS_LHA(void *, handlerData2, A2),

/*  LOCATION */
        struct KernelBase *, KernelBase, 7, Kernel)

/*  FUNCTION

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

    struct IntrNode *handle = NULL;

    D(bug("[KRN] KrnAddIRQHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));

    if (irq < IRQ_COUNT)
    {
        /* Go to supervisor mode */
        (void)goSuper();

        handle = AllocKernelMem(sizeof(struct IntrNode));
        D(bug("[KRN] handle=%012p\n", handle));

        if (handle)
        {
            handle->in_Handler = handler;
            handle->in_HandlerData = handlerData;
            handle->in_HandlerData2 = handlerData2;
            handle->in_type = it_interrupt;
            handle->in_nr = irq;

            Disable();

            ADDHEAD(&KernelBase->kb_Interrupts[irq], &handle->in_Node);

            ictl_enable_irq(irq);

            Enable();
        }

        goUser();
    }

    return handle;

    AROS_LIBFUNC_EXIT
}
