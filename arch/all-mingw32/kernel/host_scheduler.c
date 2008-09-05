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
#include <aros/libcall.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include "etask.h"
#include "kernel_intern.h"

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

#define DS(x) x

/*
 * Task dispatcher. Basically it may be the same one no matter what scheduling algorithm is used
 */
void core_Dispatch(CONTEXT *regs)
{
    struct ExecBase *SysBase = *SysBasePtr;
    struct Task *task;

    if (SysBase)
    {
        Ints_Enabled = 0;

        /* 
         * Is the list of ready tasks empty? Well, increment the idle switch cound and halt CPU.
         * It should be extended by some plugin mechanism which would put CPU and whole machine
         * into some more sophisticated sleep states (ACPI?)
         */
        while (IsListEmpty(&SysBase->TaskReady))
        {
            SysBase->IdleCount++;
            SysBase->AttnResched |= ARF_AttnSwitch;
            
            DS(printf("[KRN] TaskReady list empty. Sleeping for a while...\n"));
            /* Sleep almost forever ;) */
            
            if (SysBase->SysFlags & SFF_SoftInt)
            {
                core_Cause(SysBase);
            }
        }
    
        SysBase->DispCount++;
        
        /* Get the first task from the TaskReady list, and populate it's settings through Sysbase */
        task = (struct Task *)REMHEAD(&SysBase->TaskReady);
        SysBase->ThisTask = task;  
        SysBase->Elapsed = SysBase->Quantum;
        SysBase->SysFlags &= ~0x2000;
        task->tc_State = TS_RUN;
        SysBase->IDNestCnt = task->tc_IDNestCnt;

        DS(printf("[KRN] New task = %p (%s)\n", task, task->tc_Node.ln_Name));

        /* Handle tasks's flags */
        if (task->tc_Flags & TF_EXCEPT)
            Exception();
        
        if (task->tc_Flags & TF_LAUNCH)
        {
            task->tc_Launch(SysBase);       
        }
        
        /* Restore the task's state */
        CopyMemory(regs, GetIntETask(task)->iet_Context, sizeof(CONTEXT));
    }
    
    /* Leave interrupt and jump to the new task */
}

void core_Switch(CONTEXT *regs)
{
    struct ExecBase *SysBase = *SysBasePtr;
    struct Task *task;
    
    if (SysBase)
    {
        Ints_Enabled = 0;
    
        task = SysBase->ThisTask;
        
        DS(printf("[KRN] Old task = %p (%s)\n", task, task->tc_Node.ln_Name));
        
        /* Copy current task's context into the ETask structure */
        CopyMemory(GetIntETask(task)->iet_Context, regs, sizeof(CONTEXT));
        
        /* store IDNestCnt into tasks's structure */  
        task->tc_IDNestCnt = SysBase->IDNestCnt;
        task->tc_SPReg = (APTR)regs->Esp;
        
        /* And enable interrupts */
        SysBase->IDNestCnt = -1;
        Ints_Enabled = 1;
        
        /* TF_SWITCH flag set? Call the switch routine */
        if (task->tc_Flags & TF_SWITCH)
        {
            task->tc_Switch(SysBase);
        }
    }
    
    core_Dispatch(regs);
}


/*
 * Schedule the currently running task away. Put it into the TaskReady list 
 * in some smart way. This function is subject of change and it will be probably replaced
 * by some plugin system in the future
 */
void core_Schedule(CONTEXT *regs)
{
    struct ExecBase *SysBase = *SysBasePtr;
    struct Task *task;

    if (SysBase)
    {
        Ints_Enabled = 0;
            
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
                {
                    return;
                }
            }
        }
    
        /* 
         * If we got here, then the rescheduling is necessary. 
         * Put the task into the TaskReady list.
         */
        task->tc_State = TS_READY;
        Enqueue(&SysBase->TaskReady, task);
     }
    
    /* Select new task to run */
    core_Switch(regs);
}


/*
 * Leave the interrupt. This function recieves the register frame used to leave the supervisor
 * mode. It never returns and reschedules the task if it was asked for.
 */
void core_ExitInterrupt(CONTEXT *regs) 
{
    struct ExecBase *SysBase = *SysBasePtr;

    if (SysBase)
    {
        /* Soft interrupt requested? It's high time to do it */
        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(SysBase);
    
        /* If task switching is disabled, leave immediatelly */
        if (((char)SysBase->TDNestCnt) < 0) /* BYTE is unsigned in Windows so we have to cast */
        {
            /* 
             * Do not disturb task if it's not necessary. 
             * Reschedule only if switch pending flag is set. Exit otherwise.
             */
            if (SysBase->AttnResched & ARF_AttnSwitch)
            {
                core_Schedule(regs);
            }
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
