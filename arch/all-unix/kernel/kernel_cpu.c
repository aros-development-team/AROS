#include <exec/alerts.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <unistd.h>

#include "../exec/etask.h"

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_scheduler.h"

#define D(x)

/*
 * Task exception handler.
 * Exceptions work in a way similar to MinGW32 port. We can't manipulate stack inside
 * UNIX signal handler, because context's SP is set to a point where it was at the moment
 * when signal was caught. Signal handler uses some processes' stack space by itself, and
 * if we try to use some space below SP, we will clobber signal handler's stack.
 * In order to overcome this we disable interrupts and jump to exception handler. Since
 * interrupts are disabled, iet_Context of our task still contains original context (saved
 * at the moment of task switch). In our exception handler we already left the signal handler,
 * so we can allocate some storage on stack and place our context there. After this we call
 * exec's Exception().
 * When we return, we place our saved context back into iet_Context and cause a SysCall (SIGUSR1)
 * with a special TS_EXCEPT state. SysCall handler will know then that it needs just to dispatch
 * the same task with the saved context (see cpu_DispatchContext() routine).
 */
static void cpu_Exception(void)
{
    /* Save return context and IDNestCnt on stack */
    struct Task *task = SysBase->ThisTask;
    char nestCnt = task->tc_IDNestCnt;
    struct AROSCPUContext save;
    APTR savesp;

    /* Save original context */
    CopyMem(GetIntETask(task)->iet_Context, &save, sizeof(struct AROSCPUContext));
    savesp = task->tc_SPReg;

    Exception();

    /* Restore saved task state and resume it. Note that interrupts are
       disabled again here */
    task->tc_IDNestCnt = nestCnt;
    SysBase->IDNestCnt = nestCnt;

    /* Restore saved context */
    CopyMem(&save, GetIntETask(task)->iet_Context, sizeof(struct AROSCPUContext));
    task->tc_SPReg = savesp;

    /* This tells task switcher that we are returning from the exception */
    SysBase->ThisTask->tc_State = TS_EXCEPT;

    /* System call */
    KernelIFace.raise(SIGUSR1);
    AROS_HOST_BARRIER
}

void cpu_Switch(regs_t *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct AROSCPUContext *ctx = GetIntETask(task)->iet_Context;

    D(bug("[KRN] cpu_Switch(), task %p (%s)\n", task, task->tc_Node.ln_Name));
    D(PRINT_SC(regs));

    SAVEREGS(ctx, regs);
    ctx->errno_backup = *KernelBase->kb_PlatformData->errnoPtr;
    task->tc_SPReg = (APTR)SP(regs);
    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;
    sigset_t sigs;

    KernelIFace.SigEmptySet(&sigs);

    while (!(task = core_Dispatch()))
    {
        /* Sleep almost forever ;) */
	KernelIFace.sigsuspend(&sigs);
	AROS_HOST_BARRIER

        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT);
    }

    D(bug("[KRN] cpu_Dispatch(), task %p (%s)\n", task, task->tc_Node.ln_Name));
    cpu_DispatchContext(task, regs);
}

void cpu_DispatchContext(struct Task *task, regs_t *regs)
{
    struct AROSCPUContext *ctx = GetIntETask(task)->iet_Context;

    RESTOREREGS(ctx, regs);
    *KernelBase->kb_PlatformData->errnoPtr = ctx->errno_backup;
    SP(regs) = (IPTR)task->tc_SPReg;

    D(PRINT_SC(regs));

    if (task->tc_Flags & TF_EXCEPT)
    {
	/* Disable interrupts, otherwise we may lose saved context */
	SysBase->IDNestCnt = 0;

	/* Manipulate the current cpu context so Exec_Exception gets
	   excecuted after we leave the kernel resp. the signal handler. */
	PC(regs) = (IPTR)cpu_Exception;
    }

    /* Adjust user mode interrupts state */
    if (SysBase->IDNestCnt < 0)
	SC_ENABLE(regs);
    else
	SC_DISABLE(regs);
}
