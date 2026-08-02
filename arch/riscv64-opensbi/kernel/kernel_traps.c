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

/* What the dispatcher leaves for the scheduler, once the depth is down */
#define TRAP_DONE       0
#define TRAP_RESCHEDULE 1
#define TRAP_SYSCALL    2

static int krnTrapDispatch(struct ExceptionContext *ctx, unsigned long scause,
                           unsigned long stval);

void krnTrapHandler(struct ExceptionContext *ctx, unsigned long scause,
                    unsigned long stval)
{
    int action;

    __riscv64_trap_depth++;
    action = krnTrapDispatch(ctx, scause, stval);
    __riscv64_trap_depth--;

    if (!SysBase)
        return;

    /*
     * The scheduler is only entered once the depth is back down. It may
     * resume another task and never return here, so anything counted
     * around it stays counted for good - and a task resumed while it
     * still looks like trap context cannot take a semaphore.
     */
    if (action == TRAP_RESCHEDULE)
        core_ExitInterrupt(ctx);
    else if (action == TRAP_SYSCALL)
        core_SysCall((int)ctx->x[CTX_REG_A7], ctx);
}

static int krnTrapDispatch(struct ExceptionContext *ctx, unsigned long scause,
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
            return TRAP_RESCHEDULE;

        case SCAUSE_IRQ_SEI:
        {
            /*
             * A device. Which one has to be claimed from the PLIC,
             * and it must be completed afterwards or it never fires
             * again. Several can be pending at once.
             */
            krnHandleExternalIRQ();
            return TRAP_RESCHEDULE;
        }

        case SCAUSE_IRQ_SSI:
            /* IPIs arrive here once SMP bring-up exists */
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
        return TRAP_SYSCALL;
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

    return TRAP_DONE;
}
