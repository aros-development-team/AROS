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

    struct IntrNode *handle = NULL;
    D(bug("[KRN] KrnAddIRQHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));
#ifdef NOT_YET
    if (irq < 63)
    {
        /* Go to supervisor mode */
        goSuper(); 
        
        handle = Allocate(KernelBase->kb_SupervisorMem, sizeof(struct IntrNode));
        D(bug("[KRN]   handle=%012p\n", handle));
        
        if (handle)
        {
            handle->in_Handler = handler;
            handle->in_HandlerData = handlerData;
            handle->in_HandlerData2 = handlerData2;
            handle->in_type = it_interrupt;
            handle->in_nr = irq;
            
            Disable();
            ADDHEAD(&KernelBase->kb_Interrupts[irq], &handle->in_Node);
            
            if (irq < 32)
            {
                wrdcr(UIC0_ER, rddcr(UIC0_ER) | (0x80000000 >> irq));
            }
            else
            {
                wrdcr(UIC1_ER, rddcr(UIC1_ER) | (0x80000000 >> (irq - 32)));
                wrdcr(UIC0_ER, rddcr(UIC0_ER) | 0x00000003);
            }
            
            Enable();
        }
        
        goUser();
    }
#endif
    return handle;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnRemIRQHandler,
         AROS_LHA(void *, handle, A0),
         struct KernelBase *, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT
#ifdef NOT_YET
    struct IntrNode *h = handle;
    uint8_t irq = h->in_nr;
    
    if (h && (h->in_type == it_interrupt))
    {
        goSuper();
     
        Disable();
        REMOVE(h);
        if (IsListEmpty(&KernelBase->kb_Interrupts[irq]))
        {
            if (irq < 30)
            {
                wrdcr(UIC0_ER, rddcr(UIC0_ER) & ~(0x80000000 >> irq));
            }
            else if (irq > 31)
            {
                wrdcr(UIC1_ER, rddcr(UIC0_ER) & ~(0x80000000 >> (irq - 32)));
            }
        }
        Enable();
    
        Deallocate(KernelBase->kb_SupervisorMem, h, sizeof(struct IntrNode));
        
        goUser();
    }
#endif
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
#ifdef NOT_YET
    if (irq < 16)
    {
        /* Go to supervisor mode */
        goSuper();
        
        handle = Allocate(KernelBase->kb_SupervisorMem, sizeof(struct IntrNode));
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
        
        goUser();
    }
#endif
    return handle;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnRemExceptionHandler,
         AROS_LHA(void *, handle, A0),
         struct KernelBase *, KernelBase, 8, Kernel)
{
    AROS_LIBFUNC_INIT
#ifdef NOT_YET
    struct IntrNode *h = handle;
    
    if (h && (h->in_type == it_exception))
    {
        goSuper();
        
        Disable();
        REMOVE(h);
        Enable();
    
        Deallocate(KernelBase->kb_SupervisorMem, h, sizeof(struct IntrNode));
        
        goUser();
    }
#endif
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnCli,
         struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

/*  asm volatile("li %%r3,%0; sc"::"i"(SC_CLI):"memory","r3"); */

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void, KrnSti,
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

/*  asm volatile("li %%r3,%0; sc"::"i"(SC_STI):"memory","r3"); */

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, KrnIsSuper,
         struct KernelBase *, KernelBase, 12, Kernel)
{
    AROS_LIBFUNC_INIT
    
    return 0;
    
    AROS_LIBFUNC_EXIT
}
