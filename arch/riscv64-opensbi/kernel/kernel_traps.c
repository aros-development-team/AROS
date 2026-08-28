/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: S-mode trap handling for the opensbi-riscv64 target.
*/

#define __KERNEL_NOLIBBASE__

#include <inttypes.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <aros/riscv64/cpucontext.h>
#include <asm/cpu.h>

#include "kernel_intern.h"
#include "kernel_cpu.h"

#include <kernel_globals.h>
#include <kernel_interrupts.h>
#include <kernel_intr.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

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

/*
 * scause exception code -> m68k trap number, the currency core_Trap()
 * and exec's trap handler deal in (exec ORs in AT_DeadEnd to form the
 * ACPU_* alert). -1 is never forwarded. Explicitly signed: plain char
 * is unsigned on riscv64, which would turn -1 into 255.
 */
static const signed char exc_trap[] =
{
     3,  2,  4,  4,     /* misaligned/access ifetch, illegal insn, ebreak */
     3,  2,  3,  2,     /* load/store misalign + access faults            */
     8,  8, -1, -1,     /* environment calls                              */
     2,  2, -1,  2,     /* page faults                                    */
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

/* Instruction-side faults: sepc itself may not be readable */
#define SCAUSE_IS_IFETCH(c) ((c) == 0 || (c) == 1 || (c) == 12)

/*
 * The instruction stream around sepc, as 16-bit units (the parcel size
 * both compressed and full-size instructions are built from), with the
 * faulting parcel marked. Enough to disassemble the crash site offline
 * without the binary at hand.
 */
static void krnDumpCode(struct ExceptionContext *ctx)
{
    unsigned char *pc = (unsigned char *)(ctx->pc & ~(IPTR)1);
    int i;

    if (!pc)
        return;

    krnSBIPutStr("[trap] code  ");
    for (i = -16; i < 16; i++)
    {
        if (i == 0)
            krnSBIPutStr("\n[trap] sepc> ");
        krnSBIPutHex8(pc[i]);
        if ((i & 1) && i != 15)
            krnSBIPutStr(" ");
    }
    krnSBIPutStr("\n");
}

/*
 * Name sepc, ra and every stack word the symbol resolver can attribute
 * to a module. With no frame chain to walk (this port omits frame
 * pointers) the scan overreports, but the real call chain is in there,
 * each entry as module+symbol+offset - no load-address log needed.
 */
static void krnDumpBacktrace(struct ExceptionContext *ctx)
{
    struct KernelBase *kbase = getKernelBase();
    APTR pcs[34];
    ULONG n = 0;

    if (!kbase)
        return;

    pcs[n++] = (APTR)ctx->pc;
    if (ctx->x[0] && ctx->x[0] != ctx->pc)
        pcs[n++] = (APTR)ctx->x[0];         /* ra */

    n += KrnBacktraceFromFrame((APTR)ctx->x[1], &pcs[n], 32 - n);

    KrnPrintBacktrace("[trap] ", pcs, n);
}

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

    /*
     * Only the outermost trap enters the scheduler, the same rule other
     * ports get from the hardware privilege level they were interrupted
     * from: a trap taken while the kernel is already in a trap - which
     * includes one taken while the dispatcher itself is idling - must
     * not reschedule, or it re-enters the dispatcher from inside itself.
     * The depth is held across the call so that nesting stays visible to
     * it, and released once it is done.
     */
    if (SysBase && (__riscv64_trap_depth == 1))
    {
        if (action == TRAP_RESCHEDULE)
            core_ExitInterrupt(ctx);
        else if (action == TRAP_SYSCALL)
            core_SysCall((int)ctx->x[CTX_REG_A7], ctx);
    }

    /*
     * __riscv64_trap_depth is released at the very end of the assembly exit
     * in traps.S (after the register restore, immediately before sret), NOT
     * here: decrementing it now would leave the whole exit sequence running
     * at depth 0, so a trap taken mid-exit would be treated as the outermost
     * one, reschedule, and snapshot this half-restored exit state as a task
     * context - which later resumes with a wild sp and faults.
     */
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
            krnTimerTick(ctx);
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

        /* kernel.resource exception handlers get first refusal */
        if (getKernelBase() && (code < EXCEPTIONS_COUNT) &&
            krnRunExceptionHandlers(getKernelBase(), code, ctx))
            return TRAP_DONE;

        /*
         * Hand the crash to exec, as the other ports do: core_Trap()
         * runs the task's trap handler, which points the context at
         * exec's crash handler and returns; the normal trap exit then
         * resumes the task there and the Alert() machinery takes over.
         * Only the outermost trap can do this - a fault taken inside
         * another trap has no task context to give the crash to.
         */
        if ((__riscv64_trap_depth == 1) && getKernelBase() &&
            (code < sizeof(exc_trap)) && (exc_trap[code] != -1) &&
            core_Trap(exc_trap[code], ctx))
            return TRAP_DONE;

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

    /* Name the faulting task so a crash can be attributed to its owner
     * (e.g. a driver process vs. the network stack). FindTask(NULL) is
     * the SMP-safe way to get the running task. */
    if (SysBase)
    {
        struct Task *t = FindTask(NULL);
        if (t)
        {
            krnSBIPutStr("[trap] task '");
            krnSBIPutStr(t->tc_Node.ln_Name ? t->tc_Node.ln_Name :
                         "<unnamed>");
            krnSBIPutStr("' @ ");
            krnSBIPutHex((IPTR)t);
            krnSBIPutStr("\n");

            /* The owner's stack bounds, and whether sp had left them -
             * a runaway stack explains a trap frame landing anywhere */
            krnSBIPutStr("[trap] stack ");
            krnSBIPutHex((IPTR)t->tc_SPLower);
            krnSBIPutStr(" - ");
            krnSBIPutHex((IPTR)t->tc_SPUpper);
            krnSBIPutStr(" sp ");
            krnSBIPutHex(ctx->x[1]);            /* regnames[1] is sp */
            krnSBIPutStr(((IPTR)ctx->x[1] < (IPTR)t->tc_SPLower ||
                          (IPTR)ctx->x[1] > (IPTR)t->tc_SPUpper)
                         ? " OUTSIDE\n" : " in bounds\n");
        }
    }

    if (!SCAUSE_IS_IFETCH(scause))
        krnDumpCode(ctx);
    krnDumpBacktrace(ctx);

    krnSBIPutStr("[trap] fatal - halting hart.\n");
    for (;;)
        asm volatile("wfi");

    return TRAP_DONE;
}
