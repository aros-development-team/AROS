#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "etask.h"

#include "host_irq.h"
#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "kernel_mingw32.h"
#include "kernel_scheduler.h"
#include "kernel_syscall.h"
#include "kernel_traps.h"

#define D(x) /* This may lock up. See notes in host_intr.c */
#define DEXCEPT(x)
#define DSLEEP(x)
#define DSC(x)
#define DTRAP(x)

#define Sleep_Mode   (*KernelIFace.SleepState)
#define LastErrorPtr (*KernelIFace.LastErrorPtr)

/*
 * User-mode part of exception handling. Save context, call
 * exec handler, then resume task.
 * Note that interrupts are disabled and SysBase->IDNestCnt contains 0.
 * Real IDNestCnt count for the task is stored in its tc_IDNestCnt.
 *
 * We have to do this complex trick with disabling interrupts because
 * in Windows exception handler operates on thread's stack, this means
 * we can't modify the stack inside exception handler, i.e. we can't save
 * the context on task's stack.
 *
 * In order to overcome this we forcibly disable interrupts in core_Dispatch()
 * and make the task to jump here. After this iet_Context still contains
 * unmodified saved task context. Since we're running normally on our stack,
 * we can save the context on the stack here.
 * Inside Exception() interrupts and task switching will be enabled, so original
 * IDNestCnt will be lost. In order to prevent it we save it on the stack too.
 *
 * When we're done we pick up saved IDNestCnt from stack and raise
 * AROS_EXCEPTION_RESUME in order to jump back to the saved context.
 */
static void core_Exception()
{
    /* Save return context and IDNestCnt on stack */
    struct Task *task = SysBase->ThisTask;
    char nestCnt = task->tc_IDNestCnt;
    struct AROSCPUContext save;

    DEXCEPT(bug("[KRN] Entered exception, task 0x%p, IDNestCnt %d\n", task, SysBase->IDNestCnt));
    /* Save original context */
    CopyMem(GetIntETask(task)->iet_Context, &save, sizeof(struct AROSCPUContext));

    /* Call exec exception processing */
    Exception();

    /* Restore saved task state and resume it. Note that interrupts are
       disabled again here */
    task->tc_IDNestCnt = nestCnt;
    SysBase->IDNestCnt = nestCnt;

    D(bug("[KRN] Leaving exception, IDNestCnt %d\n", SysBase->IDNestCnt));
    KernelIFace.core_raise(AROS_EXCEPTION_RESUME, (IPTR)&save);
}

/*
 * CPU-dependent wrapper around core_Dispatch(). Handles sleep mode
 * and task exceptions
 */
static void cpu_Dispatch(CONTEXT *regs)
{
    struct Task *task = core_Dispatch();
    struct AROSCPUContext *ctx;

    if (!task)
    {
	/*There are no ready tasks and we need to go asleep */
        if (Sleep_Mode == SLEEP_MODE_OFF)
	{
	    /* This will enable interrupts in core_LeaveInterrupt() */
	    SysBase->IDNestCnt = -1;

            SysBase->IdleCount++;
            SysBase->AttnResched |= ARF_AttnSwitch;
            DSLEEP(bug("[KRN] TaskReady list empty. Sleeping for a while...\n"));

            /* We are entering sleep mode */
	    Sleep_Mode = SLEEP_MODE_PENDING;
        }
        return;
    }

    DSLEEP(if (Sleep_Mode != SLEEP_MODE_OFF) bug("[KRN] Exiting sleep mode\n");)
    Sleep_Mode = SLEEP_MODE_OFF;

    D(bug("[KRN] Dispatched task 0x%p (%s)\n", task, task->tc_Node.ln_Name));
    /* Restore the task's context */
    ctx = GetIntETask(task)->iet_Context;
    CopyMem(ctx, regs, sizeof(CONTEXT));
    *LastErrorPtr = ctx->LastError;

    /* Handle exception if requested */
    if (task->tc_Flags & TF_EXCEPT)
    {
        DEXCEPT(bug("[KRN] Exception requested for task 0x%p, return PC = 0x%p\n", task, GET_PC(ctx)));

	/* Disable interrupts, otherwise we may lose saved context */
	SysBase->IDNestCnt = 0;

	/* Make the task to jump to exception handler */
	PC(regs) = (IPTR)core_Exception;
    }
}

static inline void SaveRegs(struct Task *t, CONTEXT *regs)
{
    struct AROSCPUContext *ctx = GetIntETask(t)->iet_Context;

    /* Actually save the context */
    CopyMem(regs, ctx, sizeof(CONTEXT));
    ctx->LastError = *LastErrorPtr;

    /* Update tc_SPReg */
    t->tc_SPReg = GET_SP(ctx);
}

/*
 * Leave the interrupt. This function receives the register frame used to leave the supervisor
 * mode. It reschedules the task if it was asked for.
 */
static void core_ExitInterrupt(CONTEXT *regs)
{
    D(bug("[Scheduler] core_ExitInterrupt\n"));

    /* Soft interrupt requested? It's high time to do it */
    if (SysBase->SysFlags & SFF_SoftInt)
    {
        D(bug("[Scheduler] Causing SoftInt\n"));
        core_Cause(INTB_SOFTINT);
    }

    /* No tasks active (AROS is sleeping)? If yes, just pick up
       a new ready task (if any) */
    if (Sleep_Mode != SLEEP_MODE_OFF)
    {
        cpu_Dispatch(regs);
        return;
    }

    /* If task switching is disabled, leave immediatelly */
    D(bug("[Scheduler] TDNestCnt is %d\n", SysBase->TDNestCnt));
    if (SysBase->TDNestCnt < 0)
    {
        /* 
         * Do not disturb task if it's not necessary. 
         * Reschedule only if switch pending flag is set. Exit otherwise.
         */
        if (SysBase->AttnResched & ARF_AttnSwitch)
        {
            D(bug("[Scheduler] Rescheduling\n"));
            if (core_Schedule())
	    {
		/* core_Switch() is machine-independent, so save registers before */
		SaveRegs(SysBase->ThisTask, regs);
		core_Switch();
		cpu_Dispatch(regs);
	    }
        }
    }
}

/* This entry point is called by host-side DLL when an IRQ arrives */
void core_IRQHandler(unsigned int num, CONTEXT *regs)
{
    krnRunIRQHandlers(num);
    /* Timer IRQ is also used to emulate exec VBlank */
    if (num == INT_TIMER)
	core_Cause(INTB_VERTB);

    core_ExitInterrupt(regs);
}

/* Trap handler entry point */
int core_TrapHandler(unsigned int num, IPTR *args, CONTEXT *regs)
{
    void (*trapHandler)(unsigned long, CONTEXT *) = NULL;
    struct ExceptionTranslation *ex;

    switch (num)
    {
    case AROS_EXCEPTION_SYSCALL:
	/* It's a SysCall exception issued by core_raise() */
	DSC(bug("[KRN] SysCall 0x%04X\n", args[0]));
	switch(args[0])
	{
	case SC_SCHEDULE:
	    if (!core_Schedule())
		break;

	case SC_SWITCH:
	    /* Save registers before core_Switch()! */
	    SaveRegs(SysBase->ThisTask, regs);
	    core_Switch();

	case SC_DISPATCH:
	    cpu_Dispatch(regs);
	    break;

	case SC_CAUSE:
	    core_ExitInterrupt(regs);
//	    core_Cause(INTB_SOFTINT);
	    break;
	}

	return FALSE;

    case AROS_EXCEPTION_RESUME:
        /* Restore saved context and continue */
	CopyMem((void *)args[0], regs, sizeof(CONTEXT));
	*LastErrorPtr = ((struct AROSCPUContext *)args[0])->LastError;

	return FALSE;

    default:
	/* It's something else, likely a CPU trap */
	bug("[KRN] Exception 0x%08lX, SysBase 0x%p, KernelBase 0x%p\n", num, SysBase, KernelBase);

	/* Find out trap handler for caught task */
	if (SysBase)
	{
            struct Task *t = SysBase->ThisTask;

            if (t)
	    {
		bug("[KRN] %s 0x%p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--");
		trapHandler = t->tc_TrapCode;
		D(bug("[KRN] Task trap handler 0x%p\n", trapHandler));
	    }
	    else
		bug("[KRN] No task\n");

	    DTRAP(bug("[KRN] Exec trap handler 0x%p\n", SysBase->TaskTrapCode));
	    if (!trapHandler)
		trapHandler = SysBase->TaskTrapCode;
	}

    	PRINT_CPUCONTEXT(regs);

	/* Translate Windows exception code to CPU and exec trap numbers */
	for (ex = Traps; ex->ExceptionCode; ex++)
	{
	    if (num == ex->ExceptionCode)
		break;
	}
	DTRAP(bug("[KRN] CPU exception %d, AROS exception %d\n", ex->CPUTrap, ex->AROSTrap));

	if (ex->CPUTrap != -1)
	{
	    if (krnRunExceptionHandlers(ex->CPUTrap, regs))
		return FALSE;
	}

	if (trapHandler && (ex->AmigaTrap != -1))
	{
	    /* Call our trap handler. Note that we may return, this means that the handler has
	       fixed the problem somehow and we may safely continue */
	    DTRAP(bug("[KRN] Amiga trap %d\n", ex->AmigaTrap));
	    trapHandler(ex->AmigaTrap, regs);

	    return FALSE;
	}

	break;	
    }

    return TRUE;
}
