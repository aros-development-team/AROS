/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: S-mode trap handling for the opensbi-riscv64 target.
*/

#include <inttypes.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/riscv64/cpucontext.h>
#include <asm/cpu.h>

#include "kernel_intern.h"
#include "kernel_cpu.h"

#include <kernel_intr.h>
#include <kernel_syscall.h>

#define SCAUSE_INTERRUPT    (1UL << 63)
#define SCAUSE_BREAKPOINT   3

/* a7 carries the syscall number (see krnSysCall in kernel_cpu.h) */
#define CTX_REG_A7          14
#define SC_MAX              0x100   /* SC_REBOOT is the highest code */

static const char * const exc_names[] =
{
    "Instruction address misaligned",   /*  0 */
    "Instruction access fault",         /*  1 */
    "Illegal instruction",              /*  2 */
    "Breakpoint",                       /*  3 */
    "Load address misaligned",          /*  4 */
    "Load access fault",                /*  5 */
    "Store/AMO address misaligned",     /*  6 */
    "Store/AMO access fault",           /*  7 */
    "Environment call from U-mode",     /*  8 */
    "Environment call from S-mode",     /*  9 */
    NULL,                               /* 10 */
    NULL,                               /* 11 */
    "Instruction page fault",           /* 12 */
    "Load page fault",                  /* 13 */
    NULL,                               /* 14 */
    "Store/AMO page fault",             /* 15 */
};

static void krnDumpContext(struct ExceptionContext *ctx)
{
    static const char * const regnames[] =
    {
        "ra ", "sp ", "t0 ", "t1 ", "t2 ", "fp ", "s1 ",
        "a0 ", "a1 ", "a2 ", "a3 ", "a4 ", "a5 ", "a6 ", "a7 ",
        "s2 ", "s3 ", "s4 ", "s5 ", "s6 ", "s7 ", "s8 ", "s9 ",
        "s10", "s11", "t3 ", "t4 ", "t5 ", "t6 "
    };
    int i;

    for (i = 0; i < RISCV_REGSAVE_CNT; i++)
    {
        krnSBIPutStr(regnames[i]);
        krnSBIPutStr("=");
        krnSBIPutHex(ctx->x[i]);
        krnSBIPutStr(((i % 3) == 2) ? "\n" : "  ");
    }
    krnSBIPutStr("\npc =");
    krnSBIPutHex(ctx->pc);
    krnSBIPutStr("\n");
}

/* Trap/interrupt nesting depth, reported through KrnIsSuper() */
extern int __riscv64_trap_depth;

static void krnTrapDispatch(struct ExceptionContext *ctx, unsigned long scause,
                            unsigned long stval);

void krnTrapHandler(struct ExceptionContext *ctx, unsigned long scause,
                    unsigned long stval)
{
    __riscv64_trap_depth++;
    krnTrapDispatch(ctx, scause, stval);
    __riscv64_trap_depth--;
}

static void krnTrapDispatch(struct ExceptionContext *ctx, unsigned long scause,
                            unsigned long stval)
{
    if (scause & SCAUSE_INTERRUPT)
    {
        unsigned long irq = scause & ~SCAUSE_INTERRUPT;

        switch (irq)
        {
        case SCAUSE_IRQ_STI:
            krnTimerTick();
            /* Run the scheduler on the way out if a switch is pending */
            if (SysBase)
                core_ExitInterrupt(ctx);
            return;

        case SCAUSE_IRQ_SSI:
            /* IPIs arrive here once SMP bring-up exists */
        case SCAUSE_IRQ_SEI:
            /* External (PLIC) interrupts, routed once drivers exist */
        default:
            krnSBIPutStr("\n[trap] unexpected interrupt, code ");
            krnSBIPutDec(irq);
            krnSBIPutStr("\n");
            break;
        }
    }
    else if (scause == SCAUSE_BREAKPOINT && SysBase &&
             ctx->x[CTX_REG_A7] <= SC_MAX)
    {
        /*
         * Scheduler syscall (KrnDispatch/KrnSwitch/KrnSchedule/...):
         * ebreak with the function code in a7 (ecall belongs to SBI,
         * see kernel_cpu.h). Step over the ebreak - 2 bytes for the
         * compressed form, 4 otherwise.
         */
        ctx->pc += (*(uint16_t *)ctx->pc == 0x9002) ? 2 : 4;
        core_SysCall((int)ctx->x[CTX_REG_A7], ctx);
        return;
    }
    else
    {
        unsigned long code = scause;
        const char *name = (code < sizeof(exc_names)/sizeof(exc_names[0]))
                            ? exc_names[code] : NULL;

        krnSBIPutStr("\n[trap] ");
        if (name)
            krnSBIPutStr(name);
        else
        {
            krnSBIPutStr("Unknown exception ");
            krnSBIPutDec(code);
        }
        krnSBIPutStr("\n       sepc  = ");
        krnSBIPutHex(ctx->pc);
        krnSBIPutStr("\n       stval = ");
        krnSBIPutHex(stval);
        krnSBIPutStr("\n");
    }

    krnDumpContext(ctx);

    krnSBIPutStr("[trap] fatal - halting hart.\n");
    for (;;)
        asm volatile("wfi");
}
