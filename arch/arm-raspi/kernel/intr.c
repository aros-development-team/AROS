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
#include "kernel_debug.h"

void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
    int bank = IRQ_BANK(irq);
    unsigned int val;
    volatile unsigned int *reg = ((bank == 0) ? GPUIRQ_ENBL0 : (bank == 1) ? GPUIRQ_ENBL1 : ARMIRQ_ENBL);

    D(bug("[KRN] Enabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));
    val = *(reg);
    val |= IRQ_MASK(irq);
    *(reg) = val;
}

void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
    int bank = IRQ_BANK(irq);
    unsigned int val;
    volatile unsigned int *reg = ((bank == 0) ? GPUIRQ_DIBL0 : (bank == 1) ? GPUIRQ_DIBL1 : ARMIRQ_DIBL);

    D(bug("[KRN] Dissabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    val = *(reg);
    val |= IRQ_MASK(irq);
    *(reg) = val;
} 

void __intrhand_undef(void)
{
    register unsigned int addr;

    *gpioGPSET0 = 1<<16; // LED OFF

    asm volatile("mov %[addr], lr" : [addr] "=r" (addr) );

    krnPanic(KernelBase, "CPU Unknown Instruction @ 0x%p", (addr - 4));
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

#ifndef RASPI_VIRTMEMSUPPORT
__attribute__ ((interrupt ("ABORT"))) void __intrhand_dataabort(void)
{
    register unsigned int addr, far;
    asm volatile("mov %[addr], lr" : [addr] "=r" (addr) );
    /* Read fault address register */
    asm volatile("mrc p15, 0, %[addr], c6, c0, 0": [addr] "=r" (far) );

    *gpioGPSET0 = 1<<16; // LED OFF

    /* Routine terminates by returning to LR-4, which is the instruction
     * after the aborted one
     * GCC doesn't properly deal with data aborts in its interrupt
     * handling - no option to return to the failed instruction
     */
    krnPanic(KernelBase, "CPU Data Abort @ 0x%p, fault address: 0x%p", (addr - 4), far);
}

/* Return to this function after a prefetch abort */
__attribute__ ((interrupt ("ABORT"))) void __intrhand_prefetchabort(void)
{
    register unsigned int addr;
    asm volatile("mov %[addr], lr" : [addr] "=r" (addr) );

    *gpioGPSET0 = 1<<16; // LED OFF

    krnPanic(KernelBase, "CPU Prefetch Abort @ 0x%p", (addr - 4));
}
#else
#warning "TODO: Implement support for retrieving pages from medium, and reattempting access"
#endif

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
    *(volatile ULONG *)ARMIRQ_DIBL = ~0;
    *(volatile ULONG *)GPUIRQ_DIBL0 = ~0; 
    *(volatile ULONG *)GPUIRQ_DIBL1 = ~0;
}
