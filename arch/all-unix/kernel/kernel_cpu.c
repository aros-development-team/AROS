#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <unistd.h>

#include "../exec/etask.h"

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_scheduler.h"

/*
 * Task exception handler. Calls exec function, then jumps back
 * to kernel mode in order to resume the task
 */
static void cpu_Exception(void)
{
    Exception();

    /* This tells task switcher that we are returning from the exception */
    SysBase->ThisTask->tc_State = TS_EXCEPT;

    /* Enter the kernel. We use an endless loop just in case the
       signal handler returns us to this point for whatever reason.
    */
    KernelIFace.raise(SIGUSR1);
    AROS_HOST_BARRIER
}

void cpu_Switch(regs_t *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct AROSCPUContext *ctx = GetIntETask(task)->iet_Context;

    SAVEREGS(ctx, regs);
    ctx->errno_backup = *KernelBase->kb_PlatformData->errnoPtr;
    task->tc_SPReg = (APTR)SP(regs);
    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;
    struct AROSCPUContext *ctx;
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

    ctx = GetIntETask(task)->iet_Context;
    RESTOREREGS(ctx, regs);
    *KernelBase->kb_PlatformData->errnoPtr = ctx->errno_backup;
    SP(regs) = (IPTR)task->tc_SPReg;

    /* Adjust user mode interrupts state */
    if (SysBase->IDNestCnt < 0)
	SC_ENABLE(regs);
    else
	SC_DISABLE(regs);

    if (task->tc_Flags & TF_EXCEPT)
    {
	/* Exec_Exception will Enable() */
	Disable();

	/* Make room for the current cpu context. */
	task->tc_SPReg -= sizeof(struct AROSCPUContext);
	if (task->tc_SPReg <= task->tc_SPLower)
	    /* POW! */
	    Alert(AT_DeadEnd|AN_StackProbe);

	ctx->sc = task->tc_SPReg;
	/* Copy current cpu context. */
	CopyMem(ctx, task->tc_SPReg, sizeof(struct AROSCPUContext));

	/* Manipulate the current cpu context so Exec_Exception gets
	   excecuted after we leave the kernel resp. the signal handler. */
	SP(regs) = (IPTR)task->tc_SPReg;
	PC(regs) = (IPTR)cpu_Exception;
    }
}
