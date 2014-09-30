/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <inttypes.h>
#include <stddef.h>

#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_interrupts.h"

AROS_LH4(void *, KrnAddIRQHandler,
         AROS_LHA(uint8_t, irq, D0),
         AROS_LHA(void *, handler, A0),
         AROS_LHA(void *, handlerData, A1),
         AROS_LHA(void *, handlerData2, A2),
         struct KernelBase *, KernelBase, 7, Kernel)
{
    AROS_LIBFUNC_INIT

    struct IntrNode *handle;

    D(bug("[KRN] KrnAddIRQHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));
    handle = AllocMem(sizeof(struct IntrNode), MEMF_PUBLIC);
    D(bug("[KRN]   handle=%012p\n", handle));
        
    if (handle)
    {
        handle->in_Handler = handler;
        handle->in_HandlerData = handlerData;
        handle->in_HandlerData2 = handlerData2;
        handle->in_type = it_interrupt;
        handle->in_nr = irq;
            
        Disable();
        ADDHEAD(KernelBase->kb_Interrupts, &handle->in_Node);
        Enable();
    }
    return handle;

    AROS_LIBFUNC_EXIT
}
