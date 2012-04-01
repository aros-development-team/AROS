
#include <aros/debug.h>
#include <asm/amcc440.h>
#include <exec/lists.h>
#include <stddef.h>

#include LC_LIBDEFS_FILE
#include "kernel_intern.h"
#include "kernel_globals.h"
#include "kernel_intr.h"

void uic_handler(context_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    
    /* Get the interrupt sources */
    uint32_t uic0_sr = rddcr(UIC0_MSR);
    uint32_t uic1_sr = rddcr(UIC1_MSR);
    
    /* kernel.resource up and running? Good. */
    if (KernelBase)
    {
        /*
         * Process the interrupt sources in the priority order. Start with 
         * the external interrupt 0 (bit 0 in UIC0_SR). If the interrupt is signalled,
         * try to execute all handlers in the list
         */
        if (uic0_sr)
        {
            uint32_t mask = 0x80000000;
            uint32_t irq = 0;
            
            for (mask = 0x80000000; mask; mask >>= 1, irq++)
            {
                if (mask & uic0_sr)
                {
                    if (!IsListEmpty(&KernelBase->kb_Interrupts[irq]))
                    {
                        struct IntrNode *in, *in2;

                        ForeachNodeSafe(&KernelBase->kb_Interrupts[irq], in, in2)
                        {
                            if (in->in_Handler)
                                in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
                        }
                    }
                    else if (irq < 30){
                        D(bug("[KRN] Orphan interrupt %d occured\n", irq));
                    }
                }
            }

            for (mask = 0x80000000; mask; mask >>= 1, irq++)
            {
                if (mask & uic1_sr)
                {
                    if (!IsListEmpty(&KernelBase->kb_Interrupts[irq]))
                    {
                        struct IntrNode *in, *in2;

                        ForeachNodeSafe(&KernelBase->kb_Interrupts[irq], in, in2)
                        {
                            if (in->in_Handler)
                                in->in_Handler(in->in_HandlerData, in->in_HandlerData2);
                        }
                    }
                    else {
                        D(bug("[KRN] Orphan interrupt %d occured\n", irq));
                    }
                }
            }
        }
    }
    
    /* Acknowledge the interrupts once the handlers are done. */
    wrdcr(UIC1_SR, uic1_sr);
    wrdcr(UIC0_SR, uic0_sr | 0x00000003);
    
    core_ExitInterrupt(ctx);
}
