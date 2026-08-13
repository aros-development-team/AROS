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

/* Per-source delivery counts, for the serial debug poke: a wedged
   device shows up as a counter that stops moving */
ULONG __irq_counts[KRN_MAX_IRQ_SOURCES];

/*
 * Sources above the controller's own are raised by whoever owns them
 * (see allocirq.c) and have nothing to unmask - a bridge receiving
 * message interrupts calls KrnRunIRQ() for them once its own line has
 * been serviced. Handing one to the controller is not an error, it
 * simply is not its source, so say nothing and leave it alone.
 */
void ictl_enable_irq(uint8_t irq, struct KernelBase *kb)
{
    irqKernelBase = kb;
    if (irq <= krnPLICSourceCount())
        krnPLICEnable(irq, 1);
}

void ictl_disable_irq(uint8_t irq, struct KernelBase *kb)
{
    if (irq <= krnPLICSourceCount())
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
        if (src < KRN_MAX_IRQ_SOURCES)
            __irq_counts[src]++;

        if (irqKernelBase)
        {
            struct krnMSIController *mc = krnFindMSIController(src);

            if (mc)
            {
                /*
                 * A controller collecting messages: nothing is wired
                 * to this source but itself, and its pending register
                 * is the only thing that says which devices signalled.
                 * Clear what has arrived, then run the handlers of the
                 * sources those vectors were given.
                 */
                ULONG pending = *(volatile ULONG *)mc->status;

                if (pending)
                {
                    *(volatile ULONG *)mc->status = pending;
                    (void)*(volatile ULONG *)mc->status;

                    while (pending)
                    {
                        unsigned int vector = __builtin_ctz(pending);

                        pending &= ~(1UL << vector);
                        if (vector < mc->count)
                            krnRunIRQHandlers(irqKernelBase,
                                              (UWORD)(mc->base + vector));
                    }
                }
            }
            else
                krnRunIRQHandlers(irqKernelBase, (uint8_t)src);
        }
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
