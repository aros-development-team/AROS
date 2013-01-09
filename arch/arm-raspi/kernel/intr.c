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
//#include "syscall.h"

volatile unsigned int *irqEnable1= (unsigned int *) 0x2000b210;
volatile unsigned int *irqEnable2= (unsigned int *) 0x2000b214;
volatile unsigned int *irqEnableBasic= (unsigned int *) 0x2000b218;

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

__attribute__ ((interrupt ("SWI"))) void __intrhand_swi(void)
{
    register unsigned int addr;
    register unsigned int swi_no;
    /* Read link register into addr - contains the address of the
     * instruction after the SWI
     */
    asm volatile("mov %[addr], lr" : [addr] "=r" (addr) );

    addr -= 4;
    /* Bottom 24 bits of the SWI instruction are the SWI number */
    swi_no = *((unsigned int *)addr) & 0x00ffffff;

    *gpioGPSET0 = 1<<16; // LED OFF
    
    D(bug("[KRN] ## SWI ##\n"));
    D(bug("[KRN] Address: 0x%p  SWI number %d\n", addr, swi_no));

    while(1)
    {
        asm("mov r0,r0\n\t");
    }
}

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
extern void *_intvecs_start, *_intvecs_end;

void core_SetupIntr(void)
{
    D(bug("[KRN] Initializing exception handlers\n"));

    /* Copy vectors into place */
    memmove(&_intvecs_start, 0,
            (unsigned int)&_intvecs_end -
            (unsigned int)&_intvecs_start);

    D(bug("[KRN] Initializing IRQs\n"));

    /* Turn on interrupts */
//    asm volatile("cpsie i");
}
