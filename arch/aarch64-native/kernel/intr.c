/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    AArch64 exception handlers.
    Called from vectors.S with a pointer to the exception frame in x0.
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

#define DREGS(x)
#define DIRQ(x)
#define D(x)

/* Linker exports from vectors.S */
extern void *__aarch64_vectors;
extern void __aarch64_halt(void);

/* External handler from syscall.c */
extern void handle_syscall(void *regs);

/* Forward declarations */
void handle_undef(regs_t *regs, uint64_t esr);
void krnCheckExcStack(regs_t *regs);
void handle_dataabort(regs_t *regs, uint64_t esr);
void handle_prefetchabort(regs_t *regs, uint64_t esr);

void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
    if (__arm_arosintern.ARMI_IRQEnable)
        __arm_arosintern.ARMI_IRQEnable(irq);
}

void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
    if (__arm_arosintern.ARMI_IRQDisable)
        __arm_arosintern.ARMI_IRQDisable(irq);
}

/* AArch64 ESR_EL1 Exception Class values */
#define ESR_EC_SVC_AARCH64      0x15    /* SVC from AArch64 */
#define ESR_EC_IABT_LOWER       0x20    /* Instruction Abort from lower EL */
#define ESR_EC_IABT_CURRENT     0x21    /* Instruction Abort from current EL */
#define ESR_EC_PC_ALIGN         0x22    /* PC alignment fault */
#define ESR_EC_DABT_LOWER       0x24    /* Data Abort from lower EL */
#define ESR_EC_DABT_CURRENT     0x25    /* Data Abort from current EL */
#define ESR_EC_SP_ALIGN         0x26    /* SP alignment fault */
#define ESR_EC_UNKNOWN          0x00    /* Unknown reason */

/*
 * handle_sync: Synchronous exception handler.
 * Dispatches based on ESR_EL1 Exception Class.
 */
void handle_sync(regs_t *regs)
{
    uint64_t esr;
    uint32_t ec;

    krnCheckExcStack(regs);

    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    ec = (esr >> 26) & 0x3f;

    switch (ec)
    {
    case ESR_EC_SVC_AARCH64:
        handle_syscall(regs);
        break;

    case ESR_EC_DABT_LOWER:
    case ESR_EC_DABT_CURRENT:
        handle_dataabort(regs, esr);
        break;

    case ESR_EC_IABT_LOWER:
    case ESR_EC_IABT_CURRENT:
        handle_prefetchabort(regs, esr);
        break;

    default:
        handle_undef(regs, esr);
        break;
    }
}

/*
 * SP_EL1 watchdog (real-Pi bring-up): exception frames only ever nest
 * DOWNWARD from the first frame's ceiling, so a frame whose top lies
 * ABOVE the recorded ceiling means somebody moved SP_EL1 up -- report
 * the interrupted context the FIRST time it happens, at the moment of
 * corruption, instead of crashing much later on garbage stack data.
 */
void krnCheckExcStack(regs_t *regs)
{
    static uintptr_t sp_ceiling = 0;
    uintptr_t top = (uintptr_t)regs + AARCH64_FRAME_SIZE;

    if (!sp_ceiling)
    {
        sp_ceiling = top;
    }
    else if (top > sp_ceiling)
    {
        bug("[Kernel] !!! SP_EL1 ABOVE CEILING: frame top %p > %p\n",
            (void *)top, (void *)sp_ceiling);
        bug("[Kernel] !!! interrupted ctx: pc=%p lr=%p sp=%p spsr=%p\n",
            (void *)regs->pc, (void *)regs->lr, (void *)regs->sp,
            (void *)regs->cpsr);
        sp_ceiling = top;   /* re-arm at new level to avoid log floods */
    }
}

/*
 * handle_irq: IRQ exception handler.
 */
void handle_irq(regs_t *regs)
{
    DIRQ(bug("[Kernel] ## IRQ ##\n"));
    krnCheckExcStack(regs);

    DREGS(cpu_DumpRegs(regs));

    if (__arm_arosintern.ARMI_IRQProcess)
        __arm_arosintern.ARMI_IRQProcess();

    DIRQ(bug("[Kernel] IRQ processing finished\n"));
}

/*
 * handle_fiq: FIQ exception handler.
 */
void handle_fiq(regs_t *regs)
{
    DIRQ(bug("[Kernel] ## FIQ ##\n"));

    if (__arm_arosintern.ARMI_FIQProcess)
        __arm_arosintern.ARMI_FIQProcess();

    DIRQ(bug("[Kernel] FIQ processing finished\n"));
}

/*
 * handle_serror: SError (asynchronous abort) handler.
 */
void handle_serror(regs_t *regs)
{
    uint64_t esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));

    bug("[Kernel] SError (Asynchronous Abort)\n");
    bug("[Kernel]    ESR_EL1=0x%016lx PC=0x%016lx\n", esr, regs->pc);

    cpu_DumpRegs(regs);
    __aarch64_halt();
}

/*
 * handle_undef: Unknown/undefined instruction handler.
 */
void handle_undef(regs_t *regs, uint64_t esr)
{
    bug("[Kernel] Trap AArch64 Undefined/Unknown Exception\n");
    bug("[Kernel]    exception #4 (Illegal instruction)\n");
    bug("[Kernel]    ESR_EL1=0x%016lx EC=0x%02x at 0x%016lx\n",
        esr, (uint32_t)((esr >> 26) & 0x3f), regs->pc);

    /*
     * Bring-up diagnostic: dump the instruction stream around PC (the
     * faulting page is mapped, or we would have taken an instruction
     * abort instead). The byte pattern distinguishes a jump into
     * non-code (zeroes/data) from a mis-relocated or genuinely
     * unsupported instruction. Also dump lr and a few stack words to
     * locate the caller within the seglist.
     */
    {
        uint32_t *pc_addr = (uint32_t *)regs->pc;
        uint64_t *sp_addr = (uint64_t *)regs->sp;
        int i;

        bug("[Kernel]    code @ pc-16:");
        for (i = -4; i <= 3; i++)
            bug(" %08x", pc_addr[i]);
        bug("\n[Kernel]    lr=0x%016lx sp=0x%016lx\n", regs->lr, regs->sp);
        bug("[Kernel]    stack:");
        for (i = 0; i < 8; i++)
            bug(" %016lx", sp_addr[i]);
        bug("\n");
    }

    if (krnRunExceptionHandlers(KernelBase, 4, regs))
        return;

    D(bug("[Kernel] exception handler(s) returned\n"));

    if (core_Trap(4, regs))
    {
        D(bug("[Kernel] trap handler(s) returned\n"));
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #4\n");
    cpu_DumpRegs(regs);
    __aarch64_halt();
}

/*
 * handle_dataabort: Data abort handler.
 */
void handle_dataabort(regs_t *regs, uint64_t esr)
{
    uint64_t far;

    asm volatile("mrs %0, far_el1" : "=r"(far));

    bug("[Kernel] Trap AArch64 Data Abort Exception\n");
    bug("[Kernel]    exception #2 (Bus Error)\n");
    bug("[Kernel]    ESR_EL1=0x%016lx FAR_EL1=0x%016lx PC=0x%016lx\n", esr, far, regs->pc);

    /* Dump faulting instruction context */
    {
        uint32_t *pc_addr = (uint32_t *)(regs->pc);
        bug("[Kernel]    Faulting instruction: 0x%08x\n", *pc_addr);
        bug("[Kernel]    PC-8: %08x  PC-4: %08x  PC: %08x  PC+4: %08x  PC+8: %08x\n",
            pc_addr[-2], pc_addr[-1], pc_addr[0], pc_addr[1], pc_addr[2]);
    }

    cpu_DumpRegs(regs);

    /* Diagnostic: dump crashed task info */
    {
        tls_t *__tls = TLS_PTR_GET();
        struct Task *task = __tls ? __tls->ThisTask : NULL;

        bug("[Kernel] SysBase=0x%p\n", SysBase);

        if (task) {
            struct ETask *et;
            bug("[Kernel] Current task: '%s' @ 0x%p\n",
                task->tc_Node.ln_Name ? task->tc_Node.ln_Name : "<NULL>", task);
            et = task->tc_UnionETask.tc_ETask;
            if (et && et->et_RegFrame) {
                regs_t *ctx = (regs_t *)et->et_RegFrame;
                bug("[Kernel] ExceptionContext @ 0x%p (et_RegFrame):\n", ctx);
                bug("[Kernel]   x0-x3:  %016lx %016lx %016lx %016lx\n",
                    ctx->x[0], ctx->x[1], ctx->x[2], ctx->x[3]);
                bug("[Kernel]   fp=%016lx sp=%016lx lr=%016lx pc=%016lx cpsr=%08x\n",
                    ctx->fp, ctx->sp, ctx->lr, ctx->pc, ctx->cpsr);
            } else {
                bug("[Kernel] No ExceptionContext (et=0x%p)\n", et);
            }
        } else {
            bug("[Kernel] No current task (tls=0x%p)\n", __tls);
        }
    }

    if (krnRunExceptionHandlers(KernelBase, 2, regs))
        return;

    D(bug("[Kernel] exception handler(s) returned\n"));

    if (core_Trap(2, regs))
    {
        D(bug("[Kernel] trap handler(s) returned\n"));
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #2\n");
    cpu_DumpRegs(regs);
    __aarch64_halt();
}

/*
 * handle_prefetchabort: Instruction abort handler.
 */
void handle_prefetchabort(regs_t *regs, uint64_t esr)
{
    uint64_t far;

    asm volatile("mrs %0, far_el1" : "=r"(far));

    bug("[Kernel] Trap AArch64 Instruction Abort Exception\n");
    bug("[Kernel]    exception #3 (Address Error)\n");
    bug("[Kernel]    ESR_EL1=0x%016lx FAR_EL1=0x%016lx at 0x%016lx\n", esr, far, regs->pc);

    cpu_DumpRegs(regs);

    if (krnRunExceptionHandlers(KernelBase, 3, regs))
        return;

    D(bug("[Kernel] exception handler(s) returned\n"));

    if (core_Trap(3, regs))
    {
        D(bug("[Kernel] trap handler(s) returned\n"));
        return;
    }

    bug("[Kernel] UNHANDLED EXCEPTION #3\n");
    cpu_DumpRegs(regs);
    __aarch64_halt();
}


/* ================================================================== */
/*  Cache maintenance                                                 */
/* ================================================================== */

void aarch64_flush_cache(uintptr_t addr, uint32_t length)
{
    while (length)
    {
        asm volatile("dc civac, %0" :: "r"(addr));
        addr += 64;
        length = (length > 64) ? length - 64 : 0;
    }
    asm volatile("dsb sy" ::: "memory");
}

void aarch64_icache_invalidate(uintptr_t addr, uint32_t length)
{
    while (length)
    {
        asm volatile("ic ivau, %0" :: "r"(addr));
        addr += 64;
        length = (length > 64) ? length - 64 : 0;
    }
    asm volatile("dsb sy" ::: "memory");
    asm volatile("isb" ::: "memory");
}


/* ================================================================== */
/*  Interrupt setup                                                   */
/* ================================================================== */

void core_SetupIntr(void)
{
    D(bug("[Kernel] Initializing AArch64 exception vectors\n"));

    /* Set VBAR_EL1 to point to our vector table */
    asm volatile("msr vbar_el1, %0" :: "r"(&__aarch64_vectors));
    asm volatile("isb" ::: "memory");

    D(bug("[Kernel] VBAR_EL1 set to 0x%p\n", &__aarch64_vectors));

    if (__arm_arosintern.ARMI_IRQInit)
        __arm_arosintern.ARMI_IRQInit();
}
