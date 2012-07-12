
#include <aros/debug.h>
#include <asm/amcc440.h>
#include <exec/lists.h>
#include <stddef.h>

#include LC_LIBDEFS_FILE
#include "kernel_intern.h"
#include "kernel_globals.h"
#include "kernel_intr.h"

void uic_handler(context_t *ctx, uint8_t exception)
{
    struct KernelBase *KernelBase = getKernelBase();
    
    /* Get the interrupt sources */
    uint32_t uic_sr[4];
    uint32_t uic_tr[4];

    uint32_t uic0_cascade;
    uint32_t pvr = rdspr(PVR);

    uic_sr[0] = rddcr(UIC0_MSR);
    uic_sr[1] = rddcr(UIC1_MSR);
    uic_tr[0] = rddcr(UIC0_TR);
    uic_tr[1] = rddcr(UIC1_TR);
    if (krnIsPPC460(pvr)) {
        uic_sr[2] = rddcr(UIC2_MSR);
        uic_sr[3] = rddcr(UIC3_MSR);
        uic_tr[2] = rddcr(UIC2_TR);
        uic_tr[3] = rddcr(UIC3_TR);
        uic0_cascade = INTR_UIC0_CASCADE;
    } else {
        uic_sr[2] = 0;
        uic_sr[3] = 0;
        uic_tr[2] = 0;
        uic_tr[3] = 0;
        uic0_cascade = 0x00000003;
    }
    
    /* Acknowledge edge interrups now */
    if (krnIsPPC460(pvr)) {
        wrdcr(UIC3_SR, uic_sr[3] & uic_tr[3]);
        wrdcr(UIC2_SR, uic_sr[2] & uic_tr[2]);
    }
    wrdcr(UIC1_SR, uic_sr[1] & uic_tr[1]);
    wrdcr(UIC0_SR, uic_sr[0] & uic_tr[0]);

     /* kernel.resource up and running? Good. */
    if (KernelBase)
    {
        int i;
        uint32_t irq = 0;

        /*
         * Process the interrupt sources in the priority order.
         */
        for (i = 0; i < 4; i++) {
            uint32_t mask;

            if (uic_sr[i] == 0) {
                irq += 32;
                continue;
            }

            for (mask = 0x80000000; mask; mask >>= 1, irq++)
            {
                if (i == 0 && (mask & uic0_cascade))
                    continue;

                if (mask & uic_sr[i])
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
                }
            }
        }
    }
    
    /* Acknowledge the level interrupts once the handlers are done. */
    if (krnIsPPC460(pvr)) {
        wrdcr(UIC3_SR, uic_sr[3] & ~uic_tr[3]);
        wrdcr(UIC2_SR, uic_sr[2] & ~uic_tr[2]);
    }
    wrdcr(UIC1_SR, uic_sr[1] & ~uic_tr[1]);
    wrdcr(UIC0_SR, uic_sr[0] & ~uic_tr[0]);
    
    ExitInterrupt(ctx);
}
