/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <hardware/intbits.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"

#define IRQBANK_POINTER(bank) ((bank == 0) ? GPUIRQ_ENBL0 : (bank == 1) ? GPUIRQ_ENBL1 : ARMIRQ_ENBL)

#define DREGS(x)
#define D(x)

void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
    int bank = IRQ_BANK(irq);
    unsigned int val, reg;

    reg = IRQBANK_POINTER(bank);

    D(bug("[KRN] Enabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    val = *((volatile unsigned int *)reg);
    val |= IRQ_MASK(irq);
    *((volatile unsigned int *)reg) = val;
}

void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
    int bank = IRQ_BANK(irq);
    unsigned int val, reg;

    reg = IRQBANK_POINTER(bank);

    D(bug("[KRN] Dissabling irq %d [bank %d, reg 0x%p]\n", irq, bank, reg));

    val = *((volatile unsigned int *)reg);
    val |= IRQ_MASK(irq);
    *((volatile unsigned int *)reg) = val;
} 

/* ** UNDEF INSTRUCTION EXCEPTION ** */

asm (
    ".set	MODE_SYSTEM, 0x1f              \n"

    ".globl __vectorhand_undef                 \n"
    ".type __vectorhand_undef,%function        \n"
    "__vectorhand_undef:                       \n"
    VECTCOMMON_START
    "           cpsid   i, #MODE_SYSTEM        \n" // switch to system mode, with interrupts disabled..
    "           str     sp, [r0, #13*4]        \n"
    "           str     lr, [r0, #14*4]        \n" // store lr in ctx_lr
    "           mov     fp, #0                 \n" // clear fp

    "           bl      handle_undef           \n"

    VECTCOMMON_END
);

void handle_undef(regs_t *regs)
{
    bug("[Kernel] Trap ARM Undef Exception -> Exception #4 (Illegal instruction)\n");

    if (krnRunExceptionHandlers(KernelBase, 4, regs))
	return;

    if (core_Trap(4, regs))
    {
        bug("[Kernel] Trap handler returned\n");
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #4\n");

    while (1)
        asm volatile ("mov r0, r0\n");
}

/* ** RESET HANDLER ** */

void __vectorhand_reset(void)
{
    *(volatile unsigned int *)GPSET0 = 1<<16; // LED OFF

    D(bug("[KRN] ## RESET ##\n"));
    while(1)
    {
        asm("mov r0,r0\n\t");
    }
    return;
}

/* ** SWI HANDLER ** */

/** SWI handled in syscall.c */

/* ** IRQ HANDLER ** */

asm (
    ".set	MODE_IRQ, 0x12                 \n"
    ".set	MODE_SUPERVISOR, 0x13          \n"
    ".set	MODE_SYSTEM, 0x1f              \n"

    ".globl __vectorhand_irq                   \n"
    ".type __vectorhand_irq,%function          \n"
    "__vectorhand_irq:                         \n"
    "           sub     lr, lr, #4             \n" // adjust lr_irq
    VECTCOMMON_START
    "           cpsid   i, #MODE_SYSTEM        \n" // switch to system mode, with interrupts disabled..
    "           str     sp, [r0, #13*4]        \n"
    "           str     lr, [r0, #14*4]        \n" // store lr in ctx_lr
    "           mov     fp, #0                 \n" // clear fp

    "           bl      handle_irq             \n"

    "           cpsid   i, #MODE_IRQ           \n" // switch to IRQ mode, with interrupts disabled..
    "           mov     r0, sp                 \n"
    "           mov     fp, #0                 \n" // clear fp
    "           bl      core_ExitInterrupt     \n"
    VECTCOMMON_END
);

#define IRQ_BANK1	0x00000100
#define IRQ_BANK2	0x00000200

void handle_irq(regs_t *regs)
{
    unsigned int pending, processed, irq;

    D(bug("[KRN] ## IRQ ##\n"));

    DREGS(cpu_DumpRegs(regs));

    pending = *((volatile unsigned int *)(ARMIRQ_PEND));
    D(bug("[KRN] PendingARM %08x\n", pending));
    if (!(pending & IRQ_BANK1))
    {
        processed = 0;
        for (irq = (2 << 5); irq < ((2 << 5) + 32); irq++)
        {
            if (pending & (1 << (irq - (2 << 5))))
            {
                D(bug("[KRN] Handling IRQ %d ..\n", irq));
                krnRunIRQHandlers(KernelBase, irq);
                processed |= (1 << (irq - (2 << 5)));
            }
        }
    }
    else
    {
        processed = IRQ_BANK1;
    }
    if (processed) *((volatile unsigned int *)(ARMIRQ_PEND)) = (pending & ~processed);

    pending = *((volatile unsigned int *)(GPUIRQ_PEND0));
    D(bug("[KRN] Pending0 %08x\n", pending));
    if (!(pending & IRQ_BANK2))
    {
        processed = 0;
        for (irq = (0 << 5); irq < ((0 << 5) + 32); irq++)
        {
            if (pending & (1 << (irq - (0 << 5))))
            {
                D(bug("[KRN] Handling IRQ %d ..\n", irq));
                krnRunIRQHandlers(KernelBase, irq);
                processed |= (1 << (irq - (0 << 5)));
            }
        }
    }
    else
    {
        processed = IRQ_BANK2;
    }
    if (processed) *((volatile unsigned int *)(GPUIRQ_PEND0)) = (pending & ~processed);

    pending = *((volatile unsigned int *)(GPUIRQ_PEND1));
    D(bug("[KRN] Pending1 %08x\n", pending));
    processed = 0;
    for (irq = (1 << 5); irq < ((1 << 5) + 32); irq++)
    {
            if (pending & (1 << (irq - (1 << 5))))
        {
            D(bug("[KRN] Handling IRQ %d ..\n", irq));
            krnRunIRQHandlers(KernelBase, irq);
            processed |= (1 << (irq - (1 << 5)));
        }
    }
    if (processed) *((volatile unsigned int *)(GPUIRQ_PEND1)) = (pending & ~processed);

    D(bug("[KRN] IRQ processing finished\n"));

    return;
}

/* ** FIQ HANDLER ** */

__attribute__ ((interrupt ("FIQ"))) void __vectorhand_fiq(void)
{
    *(volatile unsigned int *)GPSET0 = 1<<16; // LED OFF

    D(bug("[KRN] ## FIQ ##\n"));
    while(1)
    {
        asm("mov r0,r0\n\t");
    }
}


/* ** DATA ABORT EXCEPTION ** */

asm (
    ".set	MODE_SYSTEM, 0x1f              \n"

    ".globl __vectorhand_dataabort             \n"
    ".type __vectorhand_dataabort,%function    \n"
    "__vectorhand_dataabort:                   \n"
    VECTCOMMON_START
    "           cpsid   i, #MODE_SYSTEM        \n" // switch to system mode, with interrupts disabled..
    "           str     sp, [r0, #13*4]        \n"
    "           str     lr, [r0, #14*4]        \n" // store lr in ctx_lr
    "           mov     fp, #0                 \n" // clear fp

    "           bl      handle_dataabort       \n"

    VECTCOMMON_END
);

void handle_dataabort(regs_t *regs)
{
    register unsigned int far;

    // Read fault address register
    asm volatile("mrc p15, 0, %[far], c6, c0, 0": [far] "=r" (far) );

    bug("[Kernel] Trap ARM Data Abort Exception -> Exception #2 (Bus Error)\n");
    bug("[Kernel] attempt to access 0x%p\n", far);

    if (krnRunExceptionHandlers(KernelBase, 2, regs))
	return;

    if (core_Trap(2, regs))
    {
        bug("[Kernel] Trap handler returned\n");
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #2\n");

    while (1)
        asm volatile ("mov r0, r0\n");
}

/* ** PREFETCH ABORT EXCEPTION ** */

asm (
    ".set	MODE_SYSTEM, 0x1f              \n"

    ".globl __vectorhand_prefetchabort         \n"
    ".type __vectorhand_prefetchabort,%function \n"
    "__vectorhand_prefetchabort:               \n"
    VECTCOMMON_START
    "           cpsid   i, #MODE_SYSTEM        \n" // switch to system mode, with interrupts disabled..
    "           str     sp, [r0, #13*4]        \n"
    "           str     lr, [r0, #14*4]        \n" // store lr in ctx_lr
    "           mov     fp, #0                 \n" // clear fp

    "           bl      handle_prefetchabort   \n"

    VECTCOMMON_END
);

void handle_prefetchabort(regs_t *regs)
{
    bug("[Kernel] Trap ARM Prefetch Abort Exception -> Exception #3 (Address Error)\n");

    if (krnRunExceptionHandlers(KernelBase, 3, regs))
	return;

    if (core_Trap(3, regs))
    {
        bug("[Kernel] Trap handler returned\n");
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #3\n");

    while (1)
        asm volatile ("mov r0, r0\n");
}


/* ** SETUP ** */

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
    *(volatile unsigned int *)ARMIRQ_DIBL = ~0;
    *(volatile unsigned int *)GPUIRQ_DIBL0 = ~0; 
    *(volatile unsigned int *)GPUIRQ_DIBL1 = ~0;
}
