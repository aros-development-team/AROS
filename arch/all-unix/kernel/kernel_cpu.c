/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <unistd.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"
#include "kernel_intr.h"
#include "kernel_scheduler.h"

#define D(x)

/*
 * Task exception handler.
 * Exceptions work in a way similar to MinGW32 port. We can't manipulate stack inside
 * UNIX signal handler, because context's SP is set to a point where it was at the moment
 * when signal was caught. Signal handler uses some processes' stack space by itself, and
 * if we try to use some space below SP, we will clobber signal handler's stack.
 * In order to overcome this we disable interrupts and jump to exception handler. Since
 * interrupts are disabled, et_RegFrame of our task still contains original context (saved
 * at the moment of task switch). In our exception handler we already left the signal handler,
 * so we can allocate some storage on stack and place our context there. After this we call
 * exec's Exception().
 * When we return, we place our saved context back into et_RegFrame and cause a SysCall (SIGUSR1)
 * with a special TS_EXCEPT state. SysCall handler will know then that it needs just to dispatch
 * the same task with the saved context (see cpu_DispatchContext() routine).
 */
static void cpu_Exception(void)
{
    struct KernelBase *KernelBase = getKernelBase();
    /* Save return context and IDNestCnt on stack */
    struct Task *task = SysBase->ThisTask;
    char nestCnt = task->tc_IDNestCnt;
    char save[KernelBase->kb_ContextSize];
    APTR savesp;

    /* Save original context */
    CopyMem(task->tc_UnionETask.tc_ETask->et_RegFrame, save, KernelBase->kb_ContextSize);
    savesp = task->tc_SPReg;

    Exception();

    /* Restore saved task state and resume it. Note that interrupts are
       disabled again here */
    task->tc_IDNestCnt = nestCnt;
    SysBase->IDNestCnt = nestCnt;

    /* Restore saved context */
    CopyMem(save, task->tc_UnionETask.tc_ETask->et_RegFrame, KernelBase->kb_ContextSize);
    task->tc_SPReg = savesp;

    /* This tells task switcher that we are returning from the exception */
    SysBase->ThisTask->tc_State = TS_EXCEPT;

    /* System call */
    KernelBase->kb_PlatformData->iface->raise(SIGUSR1);
    AROS_HOST_BARRIER
}

void cpu_Switch(regs_t *regs)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct Task *task = SysBase->ThisTask;
    struct AROSCPUContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    D(bug("[KRN] cpu_Switch(), task %p (%s)\n", task, task->tc_Node.ln_Name));
    D(PRINT_SC(regs));

    SAVEREGS(ctx, regs);
    ctx->errno_backup = *KernelBase->kb_PlatformData->errnoPtr;
    task->tc_SPReg = (APTR)SP(regs);
    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct PlatformData *pd = KernelBase->kb_PlatformData;
    struct Task *task;
    sigset_t sigs;

    /* This macro relies on 'pd' being present */
    SIGEMPTYSET(&sigs);

    while (!(task = core_Dispatch()))
    {
        /* Sleep almost forever ;) */
        KernelBase->kb_PlatformData->iface->sigsuspend(&sigs);
        AROS_HOST_BARRIER

        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1L << INTB_SOFTINT);
    }

    D(bug("[KRN] cpu_Dispatch(), task %p (%s)\n", task, task->tc_Node.ln_Name));
    cpu_DispatchContext(task, regs, pd);
}

void cpu_DispatchContext(struct Task *task, regs_t *regs, struct PlatformData *pd)
{
    struct AROSCPUContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    RESTOREREGS(ctx, regs);
    *pd->errnoPtr = ctx->errno_backup;

    D(PRINT_SC(regs));

    if (task->tc_Flags & TF_EXCEPT)
    {
        /* Disable interrupts, otherwise we may lose saved context */
        SysBase->IDNestCnt = 0;

        /* Manipulate the current cpu context so Exec_Exception gets
           excecuted after we leave the kernel resp. the signal handler. */
        PC(regs) = (IPTR)cpu_Exception;
    }

    /*
     * Adjust user mode interrupts state.
     * Brackets MUST present, these are complex macros.
     */
    if (SysBase->IDNestCnt < 0)
    {
        SC_ENABLE(regs);
    }
    else
    {
        SC_DISABLE(regs);
    }
}
