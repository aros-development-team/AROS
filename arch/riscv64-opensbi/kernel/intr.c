/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hardware interrupt plumbing for the opensbi-riscv64 target.

    kernel.resource's generic KrnAddIRQHandler() keeps the handler
    chains; what it needs from the port is a way to unmask a source and
    a way to run the chain once one fires. Both go through the PLIC.
*/

#include <inttypes.h>

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>
#include <kernel_interrupts.h>

#include "kernel_intern.h"

/*
 * The trap handler runs with no library bases to hand, so the one
 * passed in when a source is first unmasked is kept for it. Nothing
 * can fire before that happens - an unmasked source is the only kind
 * the controller will report.
 */
static struct KernelBase *irqKernelBase;

void ictl_enable_irq(uint8_t irq, struct KernelBase *kb)
{
    irqKernelBase = kb;
    krnPLICEnable(irq, 1);
}

void ictl_disable_irq(uint8_t irq, struct KernelBase *kb)
{
    krnPLICEnable(irq, 0);
}

/*
 * Called from the trap handler on a supervisor external interrupt.
 * More than one source can be waiting, and each has to be completed
 * or the controller will not raise it again.
 */
void krnHandleExternalIRQ(void)
{
    /*
     * Legacy PCI interrupts are level triggered: the source stays
     * asserted until a handler quiets the device. One that nothing
     * services would spin here forever and the machine would simply
     * stop, with no clue why. Bound the loop, and mask a source that
     * will not go quiet so the rest of the system keeps running and
     * says which one it was.
     */
    unsigned int src, last = 0, spins = 0;

    while ((src = krnPLICClaim()) != 0)
    {
        if (irqKernelBase)
            krnRunIRQHandlers(irqKernelBase, (uint8_t)src);
        krnPLICComplete(src);

        /*
         * Only one source coming back over and over is a stuck line.
         * A busy machine can legitimately deliver a long run here, so
         * count repeats of the same source rather than the length of
         * the run, and give it room to be merely busy.
         */
        spins = (src == last) ? spins + 1 : 0;
        last = src;

        if (spins > 1024)
        {
            krnPLICEnable(src, 0);
            krnSBIPutStr("[irq] source ");
            krnSBIPutDec(src);
            krnSBIPutStr(" will not clear - masked\n");
            break;
        }
    }
}
