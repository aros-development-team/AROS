/*
    Copyright © 2008-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU-specific add-ons for Windows-hosted scheduler.
          Context save, restore, and task exception handling.
    Lang: english
*/

#include <exec/execbase.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_mingw32.h"
#include "kernel_scheduler.h"

#define D(x)
#define DEXCEPT(x)
#define DSLEEP(x)

/*
 * User-mode part of exception handling. Save context, call
 * exec handler, then resume task.
 * Note that interrupts are disabled and SysBase->IDNestCnt contains 0
 * upon entry. Real IDNestCnt count for the task is stored in its tc_IDNestCnt.
 *
 * We have to do this complex trick with disabling interrupts because
 * in Windows exception handler operates on thread's stack, this means
 * we can't modify the stack inside exception handler, i.e. we can't save
 * the context on task's stack.
 *
 * In order to overcome this we forcibly disable interrupts in core_Dispatch()
 * and make the task to jump here. After this et_RegFrame still contains
 * unmodified saved task context. Since we're running normally on our stack,
 * we can save the context on the stack here.
 * Inside Exception() interrupts and task switching will be enabled, so original
 * IDNestCnt will be lost. In order to prevent it we save it on the stack too.
 *
 * When we're done we pick up saved IDNestCnt from stack and raise
 * AROS_EXCEPTION_RESUME in order to jump back to the saved context.
 */
static void cpu_Exception()
{
    /* Save return context and IDNestCnt on stack */
    struct Task *task = SysBase->ThisTask;
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;
    char nestCnt = task->tc_IDNestCnt;
    char ContextSave[KernelBase->kb_ContextSize];

    DEXCEPT(bug("[KRN] Entered exception, task 0x%p, IDNestCnt %d\n", task, SysBase->IDNestCnt));
    /* Save original context */
    CopyMem(ctx, ContextSave, sizeof(struct AROSCPUContext));
    COPY_FPU(ctx, (struct ExceptionContext *)ContextSave);

    /* Call exec exception processing */
    Exception();

    /* Restore saved task state and resume it. Note that interrupts are
       disabled again here */
    task->tc_IDNestCnt = nestCnt;
    SysBase->IDNestCnt = nestCnt;

    D(bug("[KRN] Leaving exception, IDNestCnt %d\n", SysBase->IDNestCnt));
    KernelIFace.core_raise(AROS_EXCEPTION_RESUME, (IPTR)ContextSave);
}

/* CPU-specific Switch() bits. Actually just context save. */
void cpu_Switch(CONTEXT *regs)
{
    struct Task *t = SysBase->ThisTask;
    struct AROSCPUContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;

    /* Actually save the context */
    SAVEREGS(regs, ctx);
    ctx->LastError = *LastErrorPtr;

    /* Update tc_SPReg */
    t->tc_SPReg = GET_SP(ctx);

    core_Switch();
}

/*
 * CPU-dependent wrapper around core_Dispatch(). Implements
 * context restore, CPU idle loop, and task exceptions.
 */
void cpu_Dispatch(CONTEXT *regs)
{
    struct Task *task = core_Dispatch();
    struct AROSCPUContext *ctx;

    if (!task)
    {
        /*
         * There are no ready tasks and we need to go idle.
         * Because of the way how our emulation works, we do not
         * have a real loop here, unlike most ports. Instead we
         * signal idle state and exit. Then there can be two possibilities:
         * a) we are called by our virtual machine's supervisor thread.
         *    In this case it will just not resume usermode thread, and
         *    will go on with interrupts processing.
         * b) we are called by usermode thread using core_Rise(). In this
         *    case we will continue execution of the code and hit while(Sleep_Mode);
         *    spinlock in core_Rise(). We will spin until supervisor thread interrupts
         *    us and actually puts asleep.
         * We can't implement idle loop similar to other ports here because of (b)
         * case. We would just deadlock then since interrupt processing is actually
         * disabled during Windows exception processing (which is also an interrupt
         * for us).
         */
        DSLEEP(if (!Sleep_Mode) bug("[KRN] TaskReady list empty. Sleeping for a while...\n"));

        /* This will enable interrupts in core_LeaveInterrupt() */
        SysBase->IDNestCnt = -1;

        /* We are entering sleep mode */
        Sleep_Mode = SLEEP_MODE_PENDING;

        return;
    }

    DSLEEP(if (Sleep_Mode) bug("[KRN] Exiting idle state\n");)
    Sleep_Mode = SLEEP_MODE_OFF;

    D(bug("[KRN] Dispatched task 0x%p (%s)\n", task, task->tc_Node.ln_Name));
    /* Restore the task's context */
    ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;
    RESTOREREGS(regs, ctx);
    *LastErrorPtr = ctx->LastError;

    /* Handle exception if requested */
    if (task->tc_Flags & TF_EXCEPT)
    {
        DEXCEPT(bug("[KRN] Exception requested for task 0x%p, return PC = 0x%p\n", task, PC(regs)));

        /* Disable interrupts, otherwise we may lose saved context */
        SysBase->IDNestCnt = 0;

        /* Make the task to jump to exception handler */
        PC(regs) = (IPTR)cpu_Exception;
    }
}
