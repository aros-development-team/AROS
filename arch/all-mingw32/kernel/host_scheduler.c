#include <aros/system.h>

#include <stddef.h>
#include <stdio.h>
#include <windows.h>

#define __typedef_LONG /* LONG, ULONG, WORD, BYTE and BOOL are declared in Windows headers. Looks like everything  */
#define __typedef_WORD /* is the same except BOOL. It's defined to short on AROS and to int on Windows. This means */
#define __typedef_BYTE /* that you can't use it in OS-native part of the code and can't use any AROS structure     */
#define __typedef_BOOL /* definition that contains BOOL.                                                           */
typedef unsigned AROS_16BIT_TYPE UWORD;
typedef unsigned char UBYTE;

#include <aros/libcall.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include "etask.h"
#include "exec_private.h"
#include "kernel_base.h"
#include "kernel_cpu.h"
#include "kernel_syscall.h"

/* We have to define needed exec functions here because proto/exec.h conflicts with WinAPI headers. */

#define Switch()    AROS_LC0(void *, Switch, struct ExecBase *, SysBase, 9, Exec)
#define Exception() AROS_LC0NR(void, Exception, struct ExecBase *, SysBase, 11, Exec)
#define Enqueue(arg1, arg2) AROS_LC2NR(void, Enqueue, \
				       AROS_LCA(struct List *,(arg1),A0), \
			               AROS_LCA(struct Node *,(arg2),A1), \
				       struct ExecBase *, SysBase, 45, Exec)

#define D(x)
#define DEXCEPT(x) x
#define DINT(x)
#define DS(x)
#define DSLEEP(x)

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
 * When we're done we pick up saved IDNestCnt from stack and use SC_RESUME syscall
 * in order to jump back to the saved context.
 */
static void core_Exception(void)
{
    /* Save return context and IDNestCnt on stack */
    struct Task *task = SysBase->ThisTask;
    char nestCnt = task->tc_IDNestCnt;
    struct AROSCPUContext *ctx = (struct AROSCPUContext *)GetIntETask(task)->iet_Context;
    struct AROSCPUContext save;
    ULONG_PTR resumeargs[2] = {
	SC_RESUME,
	(ULONG_PTR)&save
    };

    DEXCEPT(printf("[KRN] Entered exception, task 0x%p, return PC 0x%p, IDNestCnt %d, interrupts %d\n",
		    task, GET_PC((&ctx->regs)), SysBase->IDNestCnt, Ints_Enabled));
    CopyMemory(&save, ctx, sizeof(struct AROSCPUContext));
    /* Call exec exception processing */
    Exception();

    /* Restore saved task state and resume it. Note that interrupts are
       disabled again here */
    task->tc_IDNestCnt = nestCnt;
    SysBase->IDNestCnt = nestCnt;
    DEXCEPT(printf("[KRN] Leaving exception, IDNestCnt %d, interrupts %d\n",
		    SysBase->IDNestCnt, Ints_Enabled));
    RaiseException(AROS_EXCEPTION_SYSCALL, 0, 2, resumeargs);
}

/*
 * Task dispatcher. Basically it may be the same one no matter what scheduling algorithm is used
 */
void core_Dispatch(CONTEXT *regs, struct ExecBase *SysBase)
{
    struct Task *task;
    struct AROSCPUContext *ctx;

    D(bug("[KRN] core_Dispatch()\n"));

    /* 
     * Is the list of ready tasks empty? Well, increment the idle switch cound and stop the main thread.
     */
    if (IsListEmpty(&SysBase->TaskReady))
    {
        if (Sleep_Mode == SLEEP_MODE_OFF)
	{
            SysBase->IdleCount++;
            SysBase->AttnResched |= ARF_AttnSwitch;
            DSLEEP(bug("[KRN] TaskReady list empty. Sleeping for a while...\n"));

            /* We are entering sleep mode */
	    Sleep_Mode = SLEEP_MODE_PENDING;
        }
        return;
    }

    DSLEEP(if (Sleep_Mode) bug("[KRN] Exiting sleep mode\n");)
    Sleep_Mode = SLEEP_MODE_OFF;
    SysBase->DispCount++;

    /* Get the first task from the TaskReady list, and populate it's settings through Sysbase */
    task = (struct Task *)REMHEAD(&SysBase->TaskReady);
    SysBase->ThisTask = task;  
    SysBase->Elapsed = SysBase->Quantum;
    SysBase->SysFlags &= ~SFF_QuantumOver;
    task->tc_State = TS_RUN;
    SysBase->IDNestCnt = task->tc_IDNestCnt;

    DS(bug("[KRN] New task = %p (%s)\n", task, task->tc_Node.ln_Name));

    /* Restore the task's state */
    ctx = (struct AROSCPUContext *)GetIntETask(task)->iet_Context;
    CopyMemory(regs, ctx, sizeof(CONTEXT));
    *LastErrorPtr = ctx->LastError;

    /* Handle tasks's flags.
       We do it after restoring the context because we may modify PC */
    if (task->tc_Flags & TF_LAUNCH)
        task->tc_Launch(SysBase);

    if (task->tc_Flags & TF_EXCEPT)
    {
        /* Disable interrupts, otherwise we may lose saved context */
        SysBase->IDNestCnt = 0;
	DEXCEPT(printf("[KRN] Exception requested for task 0x%p, return PC = 0x%p\n", task, GET_PC(regs)));
	/* Make the task to jump to exception handler */
        SET_PC(regs, core_Exception);
    }

    /* Leave interrupt and jump to the new task */
}

void core_Switch(CONTEXT *regs, struct ExecBase *SysBase)
{
    struct Task *task;
    struct AROSCPUContext *ctx;

    D(bug("[KRN] core_Switch()\n"));

    task = SysBase->ThisTask;
    DS(bug("[KRN] Old task = %p (%s)\n", task, task->tc_Node.ln_Name));

    /* Notify exec.library and get saved context */
    ctx = Switch();

    /* Copy current task's context into the ETask structure */
    CopyMemory(ctx, regs, sizeof(CONTEXT));
    ctx->LastError = *LastErrorPtr;

    task->tc_SPReg = GET_SP(regs);

    core_Dispatch(regs, SysBase);
}


/*
 * Schedule the currently running task away. Put it into the TaskReady list 
 * in some smart way. This function is subject of change and it will be probably replaced
 * by some plugin system in the future
 */
void core_Schedule(CONTEXT *regs, struct ExecBase *SysBase)
{
    struct Task *task;

    D(bug("[KRN] core_Schedule()\n"));
            
    task = SysBase->ThisTask;
    
    /* Clear the pending switch flag. */
    SysBase->AttnResched &= ~ARF_AttnSwitch;
    
    /* If task has pending exception, reschedule it so that the dispatcher may handle the exception */
    if (!(task->tc_Flags & TF_EXCEPT))
    {
        char pri;

        /* Is the TaskReady empty? If yes, then the running task is the only one. Let it work */
        if (IsListEmpty(&SysBase->TaskReady))
            return;
        /* Does the TaskReady list contains tasks with priority equal or lower than current task?
         * If so, then check further... 
	 * Note that we explicitly convert ln_Pri to char because BYTE is unsigned in Windows */
	pri = ((struct Task*)GetHead(&SysBase->TaskReady))->tc_Node.ln_Pri;
        if (pri <= (char)task->tc_Node.ln_Pri)
        {
            /* If the running task did not used it's whole quantum yet, let it work */
            if (!(SysBase->SysFlags & SFF_QuantumOver))
                return;
        }
    }

    /* 
     * If we got here, then the rescheduling is necessary. 
     * Put the task into the TaskReady list.
     */
    task->tc_State = TS_READY;
    Enqueue(&SysBase->TaskReady, (struct Node *)task);
    
    /* Select new task to run */
    core_Switch(regs, SysBase);
}


/*
 * Leave the interrupt. This function receives the register frame used to leave the supervisor
 * mode. It reschedules the task if it was asked for.
 */
void core_ExitInterrupt(CONTEXT *regs) 
{
    char TDNestCnt;

    D(bug("[Scheduler] core_ExitInterrupt\n"));
    /* Soft interrupt requested? It's high time to do it */
    if (SysBase->SysFlags & SFF_SoftInt) {
        DS(bug("[Scheduler] Causing SoftInt\n"));
        core_Cause(SysBase);
    }
    /* No tasks active (AROS is sleeping)? If yes, just pick up
       a new ready task (if any) */
    if (Sleep_Mode) {
        core_Dispatch(regs, SysBase);
        return;
    }
    
    /* If task switching is disabled, leave immediatelly */
    TDNestCnt = SysBase->TDNestCnt; /* BYTE is unsigned in Windows so we can't use SysBase->TDNestCnt directly */
    DS(bug("[Scheduler] TDNestCnt is %d\n", TDNestCnt));
    if (TDNestCnt < 0)
    {
        /* 
         * Do not disturb task if it's not necessary. 
         * Reschedule only if switch pending flag is set. Exit otherwise.
         */
        if (SysBase->AttnResched & ARF_AttnSwitch)
        {
            DS(bug("[Scheduler] Rescheduling\n"));
            core_Schedule(regs, SysBase);
        }
    }
}

void core_Cause(struct ExecBase *SysBase)
{
    struct IntVector *iv = &SysBase->IntVects[INTB_SOFTINT];

    /* If the SoftInt vector in SysBase is set, call it. It will do the rest for us */
    if (iv->iv_Code)
    {
        iv->iv_Code(0, 0, 0, iv->iv_Code, SysBase);
    }
}
