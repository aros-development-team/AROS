/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Trap dispatch for native RISC-V kernels; the platform's
          traps.S calls krnTrapHandler(). Everything is printed through
          the platform's krnPutC(), which must work from the first
          trap on, before KernelBase exists, one nbug() per line:
          krnBug() (_bug.c) keeps each call's output together on SMP.
*/

#define __KERNEL_NOLIBBASE__

#include <inttypes.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <aros/riscv/cpucontext.h>
#include <asm/cpu.h>

#include "kernel_riscv.h"
#include "kernel_cpu.h"

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_globals.h>
#include <kernel_interrupts.h>
#include <kernel_intr.h>
#include <kernel_syscall.h>

#include <proto/kernel.h>

#if defined(__AROSEXEC_SMP__)
#include "kernel_ipi.h"
#endif

#define CAUSE_BREAKPOINT    3

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
    "Environment call from M-mode",     /* 11 */
    "Instruction page fault",           /* 12 */
    "Load page fault",                  /* 13 */
    NULL,                               /* 14 */
    "Store/AMO page fault",             /* 15 */
};

/*
 * Exception code -> m68k trap number, the currency core_Trap()
 * and exec's trap handler deal in (exec ORs in AT_DeadEnd to form the
 * ACPU_* alert). -1 is never forwarded. Explicitly signed: plain char
 * is unsigned on riscv, which would turn -1 into 255.
 */
static const signed char exc_trap[] =
{
     3,  2,  4,  4,     /* misaligned/access ifetch, illegal insn, ebreak */
     3,  2,  3,  2,     /* load/store misalign + access faults            */
     8,  8, -1,  8,     /* environment calls (11: M-mode kernels only)    */
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

#if (RISCV_REGSAVE_CNT % 3) != 2
#error "the last line below expects two registers"
#endif
    for (i = 0; i + 2 < RISCV_REGSAVE_CNT; i += 3)
    {
        nbug("%s=0x%p  %s=0x%p  %s=0x%p\n",
             regnames[i], (APTR)ctx->x[i],
             regnames[i + 1], (APTR)ctx->x[i + 1],
             regnames[i + 2], (APTR)ctx->x[i + 2]);
    }
    nbug("%s=0x%p  %s=0x%p  \npc =0x%p\n",
         regnames[i], (APTR)ctx->x[i],
         regnames[i + 1], (APTR)ctx->x[i + 1], (APTR)ctx->pc);
}

/* Trap/interrupt nesting depth, reported through KrnIsSuper():
   KRN_TRAPDEPTH (kernel_riscv.h) - per hart on SMP */

/* Instruction-side faults: the faulting pc itself may not be readable */
#define CAUSE_IS_IFETCH(c)  ((c) == 0 || (c) == 1 || (c) == 12)

/*
 * The instruction stream around the faulting pc, as 16-bit units (the
 * parcel size both compressed and full-size instructions are built
 * from), with the faulting parcel marked. Enough to disassemble the
 * crash site offline without the binary at hand.
 */
#define PARCELS_FMT "%02x%02x %02x%02x %02x%02x %02x%02x " \
                    "%02x%02x %02x%02x %02x%02x %02x%02x"
#define PARCELS(p)  (p)[0], (p)[1], (p)[2], (p)[3], (p)[4], (p)[5], \
                    (p)[6], (p)[7], (p)[8], (p)[9], (p)[10], (p)[11], \
                    (p)[12], (p)[13], (p)[14], (p)[15]

static void krnDumpCode(struct ExceptionContext *ctx)
{
    unsigned char *pc = (unsigned char *)(ctx->pc & ~(IPTR)1);

    if (!pc)
        return;

    nbug("[trap] code  " PARCELS_FMT " \n", PARCELS(pc - 16));
    nbug("[trap] " rv_csr_name(RV_CSR_EPC) "> " PARCELS_FMT "\n",
         PARCELS(pc));
}

/*
 * Name the faulting pc, ra and every stack word the symbol resolver can
 * attribute to a module. With no frame chain to rely on (the code may
 * omit frame pointers) the scan overreports, but the real call chain is
 * in there, each entry as module+symbol+offset - no load-address log
 * needed.
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

static int krnTrapDispatch(struct ExceptionContext *ctx, unsigned long cause,
                           unsigned long tval);

void krnTrapHandler(struct ExceptionContext *ctx, unsigned long cause,
                    unsigned long tval)
{
    int action;

    KRN_TRAPDEPTH++;
    action = krnTrapDispatch(ctx, cause, tval);

    /*
     * Only the outermost trap enters the scheduler, the same rule other
     * ports get from the hardware privilege level they were interrupted
     * from: a trap taken while the kernel is already in a trap - which
     * includes one taken while the dispatcher itself is idling - must
     * not reschedule, or it re-enters the dispatcher from inside itself.
     * The depth is held across the call so that nesting stays visible to
     * it, and released once it is done.
     */
    if (SysBase && (KRN_TRAPDEPTH == 1))
    {
        if (action == TRAP_RESCHEDULE)
            core_ExitInterrupt(ctx);
        else if (action == TRAP_SYSCALL)
            core_SysCall((int)ctx->x[CTX_REG_A7], ctx);
    }

    /*
     * The depth is released at the very end of the assembly exit
     * in traps.S (after the register restore, just before mret/sret), NOT
     * here: decrementing it now would leave the whole exit sequence running
     * at depth 0, so a trap taken mid-exit would be treated as the outermost
     * one, reschedule, and snapshot this half-restored exit state as a task
     * context - which later resumes with a wild sp and faults.
     */
}

static int krnTrapDispatch(struct ExceptionContext *ctx, unsigned long cause,
                           unsigned long tval)
{
    if (cause & RV_CAUSE_INTERRUPT)
    {
        unsigned long irq = cause & ~RV_CAUSE_INTERRUPT;

        switch (irq)
        {
        case RV_IRQ_TIMER:
            if (!__riscv_arosintern.RVI_TimerTick)
                break;
            __riscv_arosintern.RVI_TimerTick(ctx);
            /* Run the scheduler on the way out if a switch is pending */
            return TRAP_RESCHEDULE;

        case RV_IRQ_EXT:
            /*
             * A device. The platform claims it from its interrupt
             * controller and completes it, or it never fires again;
             * several can be pending at once.
             */
            if (!__riscv_arosintern.RVI_ExternalIRQ)
                break;
            __riscv_arosintern.RVI_ExternalIRQ();
            return TRAP_RESCHEDULE;

#if defined(__AROSEXEC_SMP__)
        case RV_IRQ_SOFT:
            /* An inter-processor interrupt (kernel_ipi.c) */
            core_IPIHandle();
            return TRAP_RESCHEDULE;
#endif
        }
        nbug("\n[trap] unexpected interrupt, code %lu\n", irq);
    }
    else if (cause == CAUSE_BREAKPOINT && SysBase &&
             ctx->x[CTX_REG_A7] <= SC_MAX)
    {
        /*
         * Scheduler syscall (KrnDispatch/KrnSwitch/KrnSchedule/...):
         * ebreak with the function code in a7 (see kernel_cpu.h). Step
         * over the ebreak - 2 bytes for the compressed form, 4
         * otherwise.
         */
        ctx->pc += (*(uint16_t *)ctx->pc == 0x9002) ? 2 : 4;
        return TRAP_SYSCALL;
    }
    else
    {
        unsigned long code = cause;
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
        if ((KRN_TRAPDEPTH == 1) && getKernelBase())
        {
            /*
             * At depth 1 it is task code that faulted, whatever the
             * cause: the task gets it, and the hart is not halted. A
             * cause with no trap of its own goes as an illegal
             * instruction.
             */
            int trapnum = ((code < sizeof(exc_trap)) && (exc_trap[code] != -1))
                            ? exc_trap[code] : 4;

            /*
             * Say so on serial before exec takes over: the Alert()
             * requester may never appear (input.device itself may be
             * the crashed task), and then this line is the only trace.
             */
            struct Task *t = SysBase ? FindTask(NULL) : NULL;

            nbug("[trap] task '%s' %s " rv_csr_name(RV_CSR_EPC) "=0x%p "
                 rv_csr_name(RV_CSR_TVAL) "=0x%p ra=0x%p -> exec\n",
                 (t && t->tc_Node.ln_Name) ? t->tc_Node.ln_Name : "<unknown>",
                 name ? name : "exception", (APTR)ctx->pc, (APTR)tval,
                 (APTR)ctx->x[0]);
            if (core_Trap(trapnum, ctx))
                return TRAP_DONE;
        }

        if (name)
            nbug("\n[trap] %s\n       %-5s = 0x%p\n       %-5s = 0x%p\n",
                 name, rv_csr_name(RV_CSR_EPC), (APTR)ctx->pc,
                 rv_csr_name(RV_CSR_TVAL), (APTR)tval);
        else
            nbug("\n[trap] Unknown exception %lu\n"
                 "       %-5s = 0x%p\n       %-5s = 0x%p\n",
                 code, rv_csr_name(RV_CSR_EPC), (APTR)ctx->pc,
                 rv_csr_name(RV_CSR_TVAL), (APTR)tval);
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
            nbug("[trap] task '%s' @ 0x%p\n",
                 t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "<unnamed>", t);

            /* The owner's stack bounds, and whether sp had left them -
             * a runaway stack explains a trap frame landing anywhere */
            nbug("[trap] stack 0x%p - 0x%p sp 0x%p%s",
                 t->tc_SPLower, t->tc_SPUpper,
                 (APTR)ctx->x[1],               /* regnames[1] is sp */
                 ((IPTR)ctx->x[1] < (IPTR)t->tc_SPLower ||
                  (IPTR)ctx->x[1] > (IPTR)t->tc_SPUpper)
                 ? " OUTSIDE\n" : " in bounds\n");
        }
    }

    if (!CAUSE_IS_IFETCH(cause))
        krnDumpCode(ctx);
    krnDumpBacktrace(ctx);

    nbug("[trap] fatal - halting hart.\n");
#if defined(__AROSEXEC_SMP__)
    /* It runs nothing more, but still answers the other harts */
    core_HaltCPU();
#else
    for (;;)
        asm volatile("wfi");
#endif

    return TRAP_DONE;
}
