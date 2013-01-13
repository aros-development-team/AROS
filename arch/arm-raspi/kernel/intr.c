/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"

#include "intr.h"

#define IRQ_MASK(irq)   (1 << (irq & 31))
#define IRQ_BANK(irq)   (irq >> 5)
#define IRQ_INDEX(irq)  (((irq >> 5) * 32) + (irq & 31))

IPTR __irq_handlers[96];

void IRQ_enable(ULONG irq, void *handler)
{
    int bank = IRQ_BANK(irq);
    volatile ULONG *reg = ((bank == 0) ? IRQ_ENBL3 : (bank == 1) ? IRQ_ENBL1 : IRQ_ENBL2);

    *reg |= IRQ_MASK(irq);

    __irq_handlers[IRQ_INDEX(irq)] = handler;
}

void IRQ_disable(ULONG irq)
{
    int bank = IRQ_BANK(irq);
    volatile ULONG *reg = ((bank == 0) ? IRQ_DIBL3 : (bank == 1) ? IRQ_DIBL1 : IRQ_DIBL2);

    *reg |= IRQ_MASK(irq);

     __irq_handlers[IRQ_INDEX(irq)] = NULL;
} 

void __intrhand_undef(void)
{
    *gpioGPSET0 = 1<<16; // LED OFF

    D(bug("[KRN] ## UNDEF ##\n"));
    while(1)
    {
        asm("mov r0,r0\n\t");
    }
}

void __intrhand_reset(void)
{
    *gpioGPSET0 = 1<<16; // LED OFF

    D(bug("[KRN] ## RESET ##\n"));
    while(1)
    {
        asm("mov r0,r0\n\t");
    }
    return;
}

/** SWI handled in syscall.c */

__attribute__ ((interrupt ("IRQ"))) void __intrhand_irq(void)
{
    *gpioGPSET0 = 1<<16; // LED OFF

    D(bug("[KRN] ## IRQ ##\n"));

    while(1)
    {
        asm("mov r0,r0\n\t");
    }
    return;
}

__attribute__ ((interrupt ("FIQ"))) void __intrhand_fiq(void)
{
    *gpioGPSET0 = 1<<16; // LED OFF

    D(bug("[KRN] ## FIQ ##\n"));
    while(1)
    {
        asm("mov r0,r0\n\t");
    }
}

__attribute__ ((interrupt ("ABORT"))) void __intrhand_dataabort(void)
{
    register unsigned int addr, far;
    asm volatile("mov %[addr], lr" : [addr] "=r" (addr) );
    /* Read fault address register */
    asm volatile("mrc p15, 0, %[addr], c6, c0, 0": [addr] "=r" (far) );

    *gpioGPSET0 = 1<<16; // LED OFF

    D(bug("[KRN] ## DATA ABORT ##\n"));
    D(bug("[KRN] Instruction address: 0x%p  fault address: 0x%p\n", (addr - 4), far));

    /* Routine terminates by returning to LR-4, which is the instruction
     * after the aborted one
     * GCC doesn't properly deal with data aborts in its interrupt
     * handling - no option to return to the failed instruction
     */
    while(1)
    {
        asm("mov r0,r0\n\t");
    }
}

/* Return to this function after a prefetch abort */
__attribute__ ((interrupt ("ABORT"))) void __intrhand_prefetchabort(void)
{
    register unsigned int addr;
    asm volatile("mov %[addr], lr" : [addr] "=r" (addr) );

    *gpioGPSET0 = 1<<16; // LED OFF

    D(bug("[KRN] ## PREFETCH ABORT ##\n"));
    D(bug("[KRN] Instruction address: 0x%p\n", addr));

    while(1)
    {
        asm("mov r0,r0\n\t");
    }
}

/* linker exports */
extern void *__intvecs_start, *__intvecs_end;

void core_SetupIntr(void)
{
    int irq;
    bug("[KRN] Initializing cpu vectors\n");

    /* Copy vectors into place */
    memcpy(0, &__intvecs_start,
            (unsigned int)&__intvecs_end -
            (unsigned int)&__intvecs_start);

    D(bug("[KRN] Copied %d bytes from 0x%p to 0x00000000\n", (unsigned int)&__intvecs_end - (unsigned int)&__intvecs_start, &__intvecs_start));

    D(
        unsigned int x = 0;
        bug("[KRN]: Vector dump-:");
        for (x=0; x < (unsigned int)&__intvecs_end - (unsigned int)&__intvecs_start; x++) {
            if ((x%16) == 0)
            {
                bug("\n[KRN]:     %08x:", x);
            }
            bug(" %02x", *((volatile UBYTE *)x));
        }
        bug("\n");
    )

    D(bug("[KRN] Disabling IRQs\n"));
    *(volatile ULONG *)IRQ_DIBL1 = ~0;
    *(volatile ULONG *)IRQ_DIBL2 = ~0; 
    *(volatile ULONG *)IRQ_DIBL3 = ~0;

    for (irq = 0; irq < 96; irq++)
    {
        __irq_handlers[irq] = NULL;
    }
}
