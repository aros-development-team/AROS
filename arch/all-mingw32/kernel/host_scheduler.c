#define DEBUG 0

#include <aros/system.h>
#include <windows.h>
#define __typedef_LONG /* LONG, ULONG, WORD, BYTE and BOOL are declared in Windows headers. Looks like everything  */
#define __typedef_WORD /* is the same except BOOL. It's defined to short on AROS and to int on Windows. This means */
#define __typedef_BYTE /* that you can't use it in OS-native part of the code and can't use any AROS structure     */
#define __typedef_BOOL /* definition that contains BOOL.                                                           */
typedef unsigned AROS_16BIT_TYPE UWORD;
typedef unsigned char UBYTE;

#include <stddef.h>
#include <stdio.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include "etask.h"
#include "kernel_intern.h"
#include "host_debug.h"
#include "cpucontext.h"

/* We have to redefine these flags here because including exec_intern.h causes conflicts
   between dos.h and WinAPI headers. This needs to be fixed - Pavel Fedin <sonic_amiga@rambler.ru */
#define SFB_SoftInt         5   /* There is a software interrupt */
#define SFF_SoftInt         (1L<<5)

#define ARB_AttnSwitch      7   /* Delayed Switch() pending */
#define ARF_AttnSwitch      (1L<<7)
#define ARB_AttnDispatch   15   /* Delayed Dispatch() pending */
#define ARF_AttnDispatch    (1L<<15)

/* We also have to define needed exec functions here because proto/exec.h also conflicts with
   WinAPI headers. */
#define Exception() AROS_LC0NR(void, Exception, struct ExecBase *, SysBase, 11, Exec)
#define Enqueue(arg1, arg2) AROS_LC2NR(void, Enqueue, \
				       AROS_LCA(struct List *,(arg1),A0), \
			               AROS_LCA(struct Node *,(arg2),A1), \
				       struct ExecBase *, SysBase, 45, Exec)

/*
 * Be careful with this, actually enabling this causes AROS to abort on first exception
 * because of OutputDebugString() calls. Looks like WinAPI functions love to perform stack
 * check and silently abort if they think something is wrong.
 */
#define DINT(x)
#define DS(x)
#define DSLEEP(x)



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
        if (!Sleep_Mode) {
            SysBase->IdleCount++;
            SysBase->AttnResched |= ARF_AttnSwitch;
            DSLEEP(bug("[KRN] TaskReady list empty. Sleeping for a while...\n"));
            /* We are entering sleep mode */
	    Sleep_Mode = SLEEP_MODE_PENDING;
        }
        return;
    }

    Sleep_Mode = SLEEP_MODE_OFF;
    SysBase->DispCount++;
        
    /* Get the first task from the TaskReady list, and populate it's settings through Sysbase */
    task = (struct Task *)REMHEAD(&SysBase->TaskReady);
    SysBase->ThisTask = task;  
    SysBase->Elapsed = SysBase->Quantum;
    SysBase->SysFlags &= ~0x2000;
    task->tc_State = TS_RUN;
    SysBase->IDNestCnt = task->tc_IDNestCnt;

    DS(bug("[KRN] New task = %p (%s)\n", task, task->tc_Node.ln_Name));

    /* Handle tasks's flags */
    if (task->tc_Flags & TF_EXCEPT)
	/* TODO: this is implemented completely in wrong way. Task exceptions in AmigaOS are called
	   in a user-mode in the context of its task, not from within the task scheduler in its context.
	   We should have core_Exception() which would do the job of remembering task's context somewhere,
	   adjusting it to jump to exception handler, then restore task context back upon return from
	   exception handler */
        Exception();
        
    if (task->tc_Flags & TF_LAUNCH)
    {
        task->tc_Launch(SysBase);       
    }
        
    /* Restore the task's state */
    ctx = (struct AROSCPUContext *)GetIntETask(task)->iet_Context;
    CopyMemory(regs, ctx, sizeof(CONTEXT));
    *LastErrorPtr = ctx->LastError;
        
    /* Leave interrupt and jump to the new task */
}

void core_Switch(CONTEXT *regs, struct ExecBase *SysBase)
{
    struct Task *task;
    struct AROSCPUContext *ctx;
    
    D(bug("[KRN] core_Switch()\n"));
    
    task = SysBase->ThisTask;
        
    DS(bug("[KRN] Old task = %p (%s)\n", task, task->tc_Node.ln_Name));
        
    /* Copy current task's context into the ETask structure */
    ctx = (struct AROSCPUContext *)GetIntETask(task)->iet_Context;
    CopyMemory(ctx, regs, sizeof(CONTEXT));
    ctx->LastError = *LastErrorPtr;
        
    /* store IDNestCnt into tasks's structure */  
    task->tc_IDNestCnt = SysBase->IDNestCnt;
    task->tc_SPReg = (APTR)regs->Esp;
        
    /* And enable interrupts */
    SysBase->IDNestCnt = -1;
        
    /* TF_SWITCH flag set? Call the switch routine */
    if (task->tc_Flags & TF_SWITCH)
    {
        task->tc_Switch(SysBase);
    }
    
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
        /* Is the TaskReady empty? If yes, then the running task is the only one. Let it work */
        if (IsListEmpty(&SysBase->TaskReady))
            return;
        /* Does the TaskReady list contains tasks with priority equal or lower than current task?
         * If so, then check further... */
        if (((struct Task*)GetHead(&SysBase->TaskReady))->tc_Node.ln_Pri <= task->tc_Node.ln_Pri)
        {
            /* If the running task did not used it's whole quantum yet, let it work */
            if (!(SysBase->SysFlags & 0x2000))
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
    struct ExecBase *SysBase = *SysBasePtr;
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
	core_LeaveInterrupt(SysBase);
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
    core_LeaveInterrupt(SysBase);
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
