#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <stddef.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"

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
        ADDHEAD(&KernelBase->kb_Interrupts, &handle->in_Node);
        Enable();
    }
    return handle;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnRemIRQHandler,
         AROS_LHA(void *, handle, A0),
         struct KernelBase *, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT
    struct IntrNode *h = handle;
    
    if (h && (h->in_type == it_interrupt))
    {
        Disable();
        REMOVE(h);
        Enable();
    
        FreeMem(h, sizeof(struct IntrNode));
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH4(void *, KrnAddExceptionHandler,
         AROS_LHA(uint8_t, irq, D0),
         AROS_LHA(void *, handler, A0),
         AROS_LHA(void *, handlerData, A1),
         AROS_LHA(void *, handlerData2, A2),
         struct KernelBase *, KernelBase, 7, Kernel)
{
    AROS_LIBFUNC_INIT

    struct IntrNode *handle = NULL;
    D(bug("[KRN] KrnAddExceptionHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));
    if (irq < EXCEPTIONS_NUM)
    {
        handle = AllocMem(sizeof(struct IntrNode), MEMF_PUBLIC);
        D(bug("[KRN]   handle=%012p\n", handle));
        
        if (handle)
        {
            handle->in_Handler = handler;
            handle->in_HandlerData = handlerData;
            handle->in_HandlerData2 = handlerData2;
            handle->in_type = it_exception;
            handle->in_nr = irq;
            
            Disable();
            ADDHEAD(&KernelBase->kb_Exceptions[irq], &handle->in_Node);
            Enable();
        }
    }
    return handle;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnRemExceptionHandler,
         AROS_LHA(void *, handle, A0),
         struct KernelBase *, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT
    struct IntrNode *h = handle;
    
    if (h && (h->in_type == it_exception))
    {
        Disable();
        REMOVE(h);
        Enable();
    
        FreeMem(h, sizeof(struct IntrNode));
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnCli,
         struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    KernelIFace.core_intr_disable();

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnSti,
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    KernelIFace.core_intr_enable();

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, KrnIsSuper,
         struct KernelBase *, KernelBase, 12, Kernel)
{
    AROS_LIBFUNC_INIT
    
    return KernelIFace.core_is_super() ? 1 : 0;
    
    AROS_LIBFUNC_EXIT
}

