#define __KERNEL_NOLIBBASE__

#include <proto/kernel.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "xtpic.h"

#define D(x)

AROS_LH4(void *, KrnAddIRQHandler,
         AROS_LHA(uint8_t, irq, D0),
         AROS_LHA(void *, handler, A0),
         AROS_LHA(void *, handlerData, A1),
         AROS_LHA(void *, handlerData2, A2),
         struct KernelBase *, KernelBase, 7, Kernel)
{
    AROS_LIBFUNC_INIT
    
    void *handle;

    D(bug("[Kernel] KrnAddIRQHandler(%02x, %012p, %012p, %012p):\n", irq, handler, handlerData, handlerData2));

    if (irq > 0xDE)
    	return NULL;

    /* In x86-64 IRQs are 1:1 mapper to CPU exceptions starting from 0x20 */
    handle = KrnAddExceptionHandler(irq + 0x20, handler, handlerData, handlerData2);
    D(bug("[Kernel] handle=%012p\n", handle));        

    if (handle)
    {
        if (KernelBase->kb_Exceptions[irq + 0x20].lh_Type == KBL_XTPIC)
            core_XTPIC_EnableIRQ(irq, KernelBase->kb_PlatformData);
    }

    return handle;
    
    AROS_LIBFUNC_EXIT
}
