/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: CPU-level task switching for the native riscv kernels.

    cpu_Switch() saves the interrupted context (the trap frame) into the
    current task's register frame; cpu_Dispatch() selects the next task
    and loads its register frame into the trap frame - the trap exit
    path then resumes the new task.
*/

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include <aros/riscv/cpucontext.h>
#include <asm/cpu.h>

#include <hardware/intbits.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_intr.h>
#include <kernel_scheduler.h>

#include "etask.h"

#include "kernel_riscv.h"
#include "kernel_cpu.h"

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

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
   the previous owner's registers must not leak through. fcvt.d.w from
   zero gives +0.0 on rv32 and rv64 alike (fmv.d.x is rv64 only) */
static void krnInitFPU(void)
{
    asm volatile(
        "csrw fcsr, zero\n"
        "fcvt.d.w f0,  zero\n fcvt.d.w f1,  zero\n fcvt.d.w f2,  zero\n"
        "fcvt.d.w f3,  zero\n fcvt.d.w f4,  zero\n fcvt.d.w f5,  zero\n"
        "fcvt.d.w f6,  zero\n fcvt.d.w f7,  zero\n fcvt.d.w f8,  zero\n"
        "fcvt.d.w f9,  zero\n fcvt.d.w f10, zero\n fcvt.d.w f11, zero\n"
        "fcvt.d.w f12, zero\n fcvt.d.w f13, zero\n fcvt.d.w f14, zero\n"
        "fcvt.d.w f15, zero\n fcvt.d.w f16, zero\n fcvt.d.w f17, zero\n"
        "fcvt.d.w f18, zero\n fcvt.d.w f19, zero\n fcvt.d.w f20, zero\n"
        "fcvt.d.w f21, zero\n fcvt.d.w f22, zero\n fcvt.d.w f23, zero\n"
        "fcvt.d.w f24, zero\n fcvt.d.w f25, zero\n fcvt.d.w f26, zero\n"
        "fcvt.d.w f27, zero\n fcvt.d.w f28, zero\n fcvt.d.w f29, zero\n"
        "fcvt.d.w f30, zero\n fcvt.d.w f31, zero\n");
}

void cpu_Switch(regs_t *regs)
{
    struct Task *task = GET_THIS_TASK;
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
    if ((ctx->sr & RV_STATUS_FS) == RV_STATUS_FS_DIRTY)
    {
        krnSaveFPU(ctx->fpuContext);
        ctx->Flags |= ECF_FPU;
        ctx->sr = (ctx->sr & ~RV_STATUS_FS) | RV_STATUS_FS_CLEAN;
    }

    /* TODO: save the vector state here when VS reads dirty (the
       VectorContext layout is ready, see kernel_cpu.h) */

    /*
     * Account the run segment that just ended: cycles for the usage
     * window (normalized by the platform's RVI_TimerTick), and wall
     * time for the task's cumulative CPU time.
     */
    if ((task->tc_Flags & TF_ETASK) && task->tc_UnionETask.tc_ETask &&
        __riscv_arosintern.RVI_TimebaseFreq)
    {
        struct IntETask *iet = IntETask(task->tc_UnionETask.tc_ETask);
        uint64_t freq = __riscv_arosintern.RVI_TimebaseFreq;
        uint64_t ran = krnRVReadTime() - iet->iet_private1;
        uint64_t ns;

        iet->iet_private2 += ran;

        ns = (ran / freq) * 1000000000ULL +
             ((ran % freq) * 1000000000ULL) / freq;
        iet->iet_CpuTime.tv_nsec += ns % 1000000000ULL;
        iet->iet_CpuTime.tv_sec += ns / 1000000000ULL;
        while (iet->iet_CpuTime.tv_nsec >= 1000000000)
        {
            iet->iet_CpuTime.tv_nsec -= 1000000000;
            iet->iet_CpuTime.tv_sec++;
        }
    }

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;

    while (!(task = core_Dispatch()))
    {
        /*
         * Nothing to run at all - the idle task should normally keep
         * this from happening, so this is only the window before it
         * exists. Sleep with traps open until one arrives; the time
         * spent here counts as this CPU's idle time, as in
         * krnRVIdleWait().
         */
        uint32_t cpu = krnRVCPUNumber();
        UQUAD idlefrom = krnRVReadTime();

        csr_set(RV_CSR_STATUS, RV_STATUS_IE);
        asm volatile("wfi");
        csr_clear(RV_CSR_STATUS, RV_STATUS_IE);

        /* Returns with traps masked: the tick below and core_Dispatch() are next */
        core_RunSoftInts();

        if (cpu < RISCV_MAXCPUS)
            __riscv_cpu_idletime[cpu] += krnRVReadTime() - idlefrom;

        if ((csr_read(RV_CSR_IP) & RV_IP_TIMER) &&
            __riscv_arosintern.RVI_TimerTick)
            __riscv_arosintern.RVI_TimerTick(NULL);
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

    /* The task starts running now - stamp its launch time */
    if ((task->tc_Flags & TF_ETASK) && task->tc_UnionETask.tc_ETask)
    {
        IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 = krnRVReadTime();
#if defined(__AROSEXEC_SMP__)
        /* Where it runs: Signal()/SetTaskPri() aim their IPIs by this */
        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber = krnRVCPUNumber();
#endif
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
