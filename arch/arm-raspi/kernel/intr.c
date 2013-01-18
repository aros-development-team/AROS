/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <hardware/intbits.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"

extern void core_Cause(unsigned char, unsigned int);

#define IRQBANK_POINTER(bank) ((bank == 0) ? GPUIRQ_ENBL0 : (bank == 1) ? GPUIRQ_ENBL1 : ARMIRQ_ENBL)

#define DREGS(x) x

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

asm (".globl __intrhand_irq\n\t"
    ".type __intrhand_irq,%function\n"
    "__intrhand_irq:\n"
    "           sub     lr, lr, #4             \n" // save return address and callers cpsr into system mode stack
    "           srsdb   #0x1f!                 \n" 
    "           cps     #0x1f                  \n" // switch to system mode
    "           sub     sp, sp, #3*4           \n" // make space to store callers lr, sp, and ip
    "           stmfd   sp!, {r0-r11}          \n" // store untouched registers to pass to c handler ..
    "           mov     r0, sp                 \n" // r0 = registers r0-r12 on the stack
    "           str     ip, [sp, #12*4]        \n"
    "           add     ip, sp, #16*4          \n" // store callers stack pointer ..
    "           str     ip, [sp, #13*4]        \n"
    "           str     lr, [sp, #14*4]        \n"
    "           ldr     r1, [sp, #1*4]         \n" // restore r1 ..
    "           ldr     r2, [sp, #2*4]         \n" // .. and r2 ..
    "           mov     fp, #0                 \n" // clear fp(??)
    "           bl      handle_irq             \n"
    "           ldr     ip, [sp, #12*4]        \n" // get task_ip
    "           ldr     lr, [sp, #14*4]        \n" // get task_lr
    "           ldr     r2, [sp, #16*4]        \n" // restore task_cpsr
    "           msr     cpsr, r2               \n"
    "           ldmfd   sp!, {r0-r11}          \n" // restore remaining task_registers
    "           add     sp, sp, #3*4           \n" // correct the stack pointer .. 
    "           rfefd   sp!                    \n" // ..and return
);

void handle_irq(void *regs)
{
    struct ExceptionContext *ctx;
    struct Task *thisTask;
    unsigned int pending, processed, irq;

    D(bug("[KRN] ## IRQ ##\n"));

    if ((thisTask = SysBase->ThisTask) != NULL)
    {
        D(bug("[KRN] IRQ invoked in '%s'", thisTask->tc_Node.ln_Name));
        if ((ctx = thisTask->tc_UnionETask.tc_ETask->et_RegFrame) != NULL)
        {
            int i;
            
            D(bug(", ExceptionContext @ 0x%p", ctx));
            DREGS(bug("\n"));
            for (i = 0; i < 12; i++)
            {
                ctx->r[i] = ((uint32_t *)regs)[i];
                DREGS(bug("[KRN]      r%02d: 0x%08x\n", i, ctx->r[i]));
            }
            ctx->ip = ((uint32_t *)regs)[12];
            DREGS(bug("[KRN] (ip) r12: 0x%08x\n", ctx->ip));
            ctx->sp = ((uint32_t *)regs)[13];
            DREGS(bug("[KRN] (sp) r13: 0x%08x\n", ctx->sp));
            ctx->lr = ((uint32_t *)regs)[14];
            DREGS(bug("[KRN] (lr) r14: 0x%08x\n", ctx->lr));
            ctx->pc = ((uint32_t *)regs)[15];
            DREGS(bug("[KRN] (pc) r15: 0x%08x\n", ctx->pc));
            ctx->cpsr = ((uint32_t *)regs)[16];
            DREGS(bug("[KRN]     cpsr: 0x%08x", ctx->cpsr));
            thisTask->tc_SPReg = ctx->sp;
        }
        D(bug("\n"));
    }

    pending = *((volatile unsigned int *)(GPUIRQ_PEND0));
    irq = (0 << 5);
    processed = 0;
    while (pending != NULL)
    {
        if (pending & 1)
        {
            D(bug("[KRN] Handling IRQ %d ..\n", irq));
            krnRunIRQHandlers(KernelBase, irq);
            processed |= (1 << irq);
        }
        pending = pending >> 1;
        irq++;
    }
    if (processed) *((volatile unsigned int *)(GPUIRQ_PEND0)) |= ~processed;

    pending = *((volatile unsigned int *)(GPUIRQ_PEND1));
    irq = (1 << 5);
    processed = 0;
    while (pending != NULL)
    {
        if (pending & 1)
        {
            D(bug("[KRN] Handling IRQ %d ..\n", irq));
            krnRunIRQHandlers(KernelBase, irq);
            processed |= (1 << irq);
        }
        pending = pending >> 1;
        irq++;
    }
    if (processed) *((volatile unsigned int *)(GPUIRQ_PEND1)) |= ~processed;

    pending = *((volatile unsigned int *)(ARMIRQ_PEND));
    irq = (2 << 5);
    processed = 0;
    while (pending != NULL)
    {
        if (pending & 1)
        {
            D(bug("[KRN] Handling IRQ %d ..\n", irq));
            krnRunIRQHandlers(KernelBase, irq);
            processed |= (1 << irq);
        }
        pending = pending >> 1;
        irq++;
    }
    if (processed) *((volatile unsigned int *)(ARMIRQ_PEND)) |= ~processed;

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

void GPUSysTimerHandler(unsigned int timerno, void *unused1)
{
    D(bug("[KRN] GPUSysTimerHandler()\n"));
    /* Signal the Exec VBlankServer */
    if (SysBase && (SysBase->IDNestCnt < 0)) {
        core_Cause(INTB_VERTB, 1L << INTB_VERTB);
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
    *(volatile ULONG *)ARMIRQ_DIBL = ~0;
    *(volatile ULONG *)GPUIRQ_DIBL0 = ~0; 
    *(volatile ULONG *)GPUIRQ_DIBL1 = ~0;
}
