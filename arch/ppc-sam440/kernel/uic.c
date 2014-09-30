/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <asm/amcc440.h>
#include <exec/lists.h>
#include <stddef.h>

#include LC_LIBDEFS_FILE
#include "kernel_intern.h"
#include "kernel_globals.h"
#include "kernel_interrupts.h"

#include "kernel_intr.h"

/*
 * Interrupt controller functions
 */

ULONG uic_er[4];

void uic_enable(int irq)
{
    ULONG mask = (0x80000000 >> (irq & 0x1f));

    uic_er[irq >> 5] |= mask;

    switch (irq >> 5) {
    case 0: wrdcr(UIC0_ER, uic_er[0]); break;
    case 1: wrdcr(UIC1_ER, uic_er[1]); break;
    case 2: wrdcr(UIC2_ER, uic_er[2]); break;
    case 3: wrdcr(UIC3_ER, uic_er[3]); break;
    default: break;
    }
}

void uic_disable(int irq)
{
    ULONG mask = (0x80000000 >> (irq & 0x1f));

    uic_er[irq >> 5] &= ~mask;

    switch (irq >> 5) {
    case 0: wrdcr(UIC0_ER, uic_er[0]); break;
    case 1: wrdcr(UIC1_ER, uic_er[1]); break;
    case 2: wrdcr(UIC2_ER, uic_er[2]); break;
    case 3: wrdcr(UIC3_ER, uic_er[3]); break;
    default: break;
    }
}

void uic_init(void)
{
    /* Disable external interrupts completely */
    if (krnIsPPC460(rdspr(PVR))) {
        uic_er[0] = INTR_UIC0_CASCADE;
        wrdcr(UIC0_ER, uic_er[0]);
        wrdcr(UIC0_PR, INTR_UIC0_POLARITY);
        wrdcr(UIC0_CR, INTR_UIC0_CRITICAL);
        wrdcr(UIC0_TR, INTR_UIC0_TRIGGER);
        wrdcr(UIC0_SR, 0xffffffff);
        wrdcr(UIC0_VCR, 0);

        uic_er[1] = 0;
        wrdcr(UIC1_ER, uic_er[1]);
        wrdcr(UIC1_PR, INTR_UIC1_POLARITY);
        wrdcr(UIC1_CR, INTR_UIC1_CRITICAL);
        wrdcr(UIC1_TR, INTR_UIC1_TRIGGER);
        wrdcr(UIC1_SR, 0xffffffff);
        wrdcr(UIC1_VCR, 0);

        uic_er[2] = 0;
        wrdcr(UIC2_ER, uic_er[2]);
        wrdcr(UIC2_PR, INTR_UIC2_POLARITY);
        wrdcr(UIC2_CR, INTR_UIC2_CRITICAL);
        wrdcr(UIC2_TR, INTR_UIC2_TRIGGER);
        wrdcr(UIC2_SR, 0xffffffff);
        wrdcr(UIC2_VCR, 0);

        uic_er[3] = 0;
        wrdcr(UIC3_ER, uic_er[3]);
        wrdcr(UIC3_PR, INTR_UIC3_POLARITY);
        wrdcr(UIC3_CR, INTR_UIC3_CRITICAL);
        wrdcr(UIC3_TR, INTR_UIC3_TRIGGER);
        wrdcr(UIC3_SR, 0xffffffff);
        wrdcr(UIC3_VCR, 0);
    } else {
        wrdcr(UIC0_ER, 0);
        wrdcr(UIC1_ER, 0);
    }
}

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
                    krnRunIRQHandlers(KernelBase, irq);
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
