/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: CPU-level task switching for the opensbi-riscv64 target.

    cpu_Switch() saves the interrupted context (the trap frame) into the
    current task's register frame; cpu_Dispatch() selects the next task
    and loads its register frame into the trap frame - the trap exit
    path then resumes the new task via sret.
*/

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <aros/riscv64/cpucontext.h>
#include <asm/cpu.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_scheduler.h>

#include "kernel_intern.h"
#include "kernel_cpu.h"

/*
 * Copy the volatile part of a context: the x registers, pc and sr.
 * Flags and the fpuContext/vecContext pointers are deliberately
 * preserved - they are metadata owned by the saved context.
 */
static void copyContext(struct ExceptionContext *dst,
                        struct ExceptionContext *src)
{
    int i;

    for (i = 0; i < RISCV_REGSAVE_CNT; i++)
        dst->x[i] = src->x[i];
    dst->pc = src->pc;
    dst->sr = src->sr;
}

static void krnSaveFPU(struct FpuContext *fpu)
{
    unsigned long fcsr;

    asm volatile(
        "fsd f0,  0(%1)  \n fsd f1,  8(%1)  \n fsd f2,  16(%1) \n"
        "fsd f3,  24(%1) \n fsd f4,  32(%1) \n fsd f5,  40(%1) \n"
        "fsd f6,  48(%1) \n fsd f7,  56(%1) \n fsd f8,  64(%1) \n"
        "fsd f9,  72(%1) \n fsd f10, 80(%1) \n fsd f11, 88(%1) \n"
        "fsd f12, 96(%1) \n fsd f13, 104(%1)\n fsd f14, 112(%1)\n"
        "fsd f15, 120(%1)\n fsd f16, 128(%1)\n fsd f17, 136(%1)\n"
        "fsd f18, 144(%1)\n fsd f19, 152(%1)\n fsd f20, 160(%1)\n"
        "fsd f21, 168(%1)\n fsd f22, 176(%1)\n fsd f23, 184(%1)\n"
        "fsd f24, 192(%1)\n fsd f25, 200(%1)\n fsd f26, 208(%1)\n"
        "fsd f27, 216(%1)\n fsd f28, 224(%1)\n fsd f29, 232(%1)\n"
        "fsd f30, 240(%1)\n fsd f31, 248(%1)\n"
        "csrr %0, fcsr\n"
        : "=r"(fcsr) : "r"(fpu->f) : "memory");

    fpu->fcsr = fcsr;
}

static void krnRestoreFPU(struct FpuContext *fpu)
{
    asm volatile(
        "csrw fcsr, %1\n"
        "fld f0,  0(%0)  \n fld f1,  8(%0)  \n fld f2,  16(%0) \n"
        "fld f3,  24(%0) \n fld f4,  32(%0) \n fld f5,  40(%0) \n"
        "fld f6,  48(%0) \n fld f7,  56(%0) \n fld f8,  64(%0) \n"
        "fld f9,  72(%0) \n fld f10, 80(%0) \n fld f11, 88(%0) \n"
        "fld f12, 96(%0) \n fld f13, 104(%0)\n fld f14, 112(%0)\n"
        "fld f15, 120(%0)\n fld f16, 128(%0)\n fld f17, 136(%0)\n"
        "fld f18, 144(%0)\n fld f19, 152(%0)\n fld f20, 160(%0)\n"
        "fld f21, 168(%0)\n fld f22, 176(%0)\n fld f23, 184(%0)\n"
        "fld f24, 192(%0)\n fld f25, 200(%0)\n fld f26, 208(%0)\n"
        "fld f27, 216(%0)\n fld f28, 224(%0)\n fld f29, 232(%0)\n"
        "fld f30, 240(%0)\n fld f31, 248(%0)\n"
        : : "r"(fpu->f), "r"((unsigned long)fpu->fcsr) : "memory");
}

/* Load the canonical clean FPU state for a task that never used FP -
   the previous owner's registers must not leak through */
static void krnInitFPU(void)
{
    asm volatile(
        "csrw fcsr, zero\n"
        "fmv.d.x f0,  zero\n fmv.d.x f1,  zero\n fmv.d.x f2,  zero\n"
        "fmv.d.x f3,  zero\n fmv.d.x f4,  zero\n fmv.d.x f5,  zero\n"
        "fmv.d.x f6,  zero\n fmv.d.x f7,  zero\n fmv.d.x f8,  zero\n"
        "fmv.d.x f9,  zero\n fmv.d.x f10, zero\n fmv.d.x f11, zero\n"
        "fmv.d.x f12, zero\n fmv.d.x f13, zero\n fmv.d.x f14, zero\n"
        "fmv.d.x f15, zero\n fmv.d.x f16, zero\n fmv.d.x f17, zero\n"
        "fmv.d.x f18, zero\n fmv.d.x f19, zero\n fmv.d.x f20, zero\n"
        "fmv.d.x f21, zero\n fmv.d.x f22, zero\n fmv.d.x f23, zero\n"
        "fmv.d.x f24, zero\n fmv.d.x f25, zero\n fmv.d.x f26, zero\n"
        "fmv.d.x f27, zero\n fmv.d.x f28, zero\n fmv.d.x f29, zero\n"
        "fmv.d.x f30, zero\n fmv.d.x f31, zero\n");
}

void cpu_Switch(regs_t *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    copyContext(ctx, regs);
    task->tc_SPReg = (APTR)regs->sp;

    /*
     * Lazy FPU switching: the task's FS state was captured into sr at
     * trap entry. Only a dirty FPU needs saving; resume Clean so the
     * next modification is tracked again.
     *
     * NOTE: this trusts that no FP code runs in the trap handler before
     * cpu_Switch() - kernel objects should eventually be built FP-free
     * to guarantee it.
     */
    if ((ctx->sr & SSTATUS_FS) == SSTATUS_FS_DIRTY)
    {
        krnSaveFPU(ctx->fpuContext);
        ctx->Flags |= ECF_FPU;
        ctx->sr = (ctx->sr & ~SSTATUS_FS) | SSTATUS_FS_CLEAN;
    }

    /* TODO: save the vector state here when VS reads dirty (the
       VectorContext layout is ready, see kernel_cpu.h) */

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;

    while (!(task = core_Dispatch()))
    {
        /*
         * Nothing to run. Interrupts are masked inside the trap
         * handler, so wait for one and service a pending timer tick by
         * hand; the wakeup it causes (via Signal from a future
         * interrupt handler) makes a task ready.
         */
        asm volatile("wfi");
        if (csr_read(sip) & SIE_STIE)
            krnTimerTick();
    }

    copyContext(regs, task->tc_UnionETask.tc_ETask->et_RegFrame);

    /*
     * Bring in the task's FPU state: restore it if it ever used FP,
     * otherwise present the canonical clean state (the previous
     * owner's registers must not leak through).
     */
    {
        struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

        if (ctx->Flags & ECF_FPU)
            krnRestoreFPU(ctx->fpuContext);
        else
            krnInitFPU();
    }

    /* Handle task's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

    if (task->tc_Flags & TF_LAUNCH)
    {
        AROS_UFC1(void, task->tc_Launch,
                  AROS_UFCA(struct ExecBase *, SysBase, A6));
    }
    /* Leave the trap and resume the new task */
}
