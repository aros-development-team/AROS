#include <asm/amcc440.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include "kernel_intern.h"
#include "exec_intern.h"
#include "etask.h"
#include "syscall.h"

AROS_LH0(KRN_SchedType, KrnGetScheduler,
         struct KernelBase *, KernelBase, 1, Kernel)
{
    AROS_LIBFUNC_INIT
    
    return SCHED_RR;
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnSetScheduler,
         AROS_LHA(KRN_SchedType, sched, D0),
         struct KernelBase *, KernelBase, 2, Kernel)
{
    AROS_LIBFUNC_INIT

    /* Cannot set scheduler yet */
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, KrnCause,
         struct KernelBase *, KernelBase, 3, Kernel)
{
    AROS_LIBFUNC_INIT
    
    asm volatile("li %%r3,%0; sc"::"i"(SC_CAUSE):"memory","r3");

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void , KrnDispatch,
         struct KernelBase *, KernelBase, 4, Kernel)
{
    AROS_LIBFUNC_INIT
    

    asm volatile("li %%r3,%0; sc"::"i"(SC_DISPATCH):"memory","r3");

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, KrnSwitch,
         struct KernelBase *, KernelBase, 5, Kernel)
{
    AROS_LIBFUNC_INIT

    asm volatile("li %%r3,%0; sc"::"i"(SC_SWITCH):"memory","r3");
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, KrnSchedule,
         struct KernelBase *, KernelBase, 6, Kernel)
{
    AROS_LIBFUNC_INIT

    asm volatile("li %%r3,%0; sc"::"i"(SC_SCHEDULE):"memory","r3");
        
    AROS_LIBFUNC_EXIT
}


/*
 * Task dispatcher. Basically it may be the same one no matter what scheduling algorithm is used
 */
void core_Dispatch(regs_t *regs)
{
    struct ExecBase *SysBase = getSysBase();
    struct Task *task;

    if (SysBase)
    {
        __asm__ __volatile__("wrteei 0;");
        
        /* 
         * Is the list of ready tasks empty? Well, increment the idle switch cound and halt CPU.
         * It should be extended by some plugin mechanism which would put CPU and whole machine
         * into some more sophisticated sleep states (ACPI?)
         */
        while (IsListEmpty(&SysBase->TaskReady))
        {
            SysBase->IdleCount++;
            SysBase->AttnResched |= ARF_AttnSwitch;
            
            /* Sleep almost forever ;) */
            __asm__ __volatile__("wrteei 1");
            wrmsr(rdmsr() | MSR_POW);
            __asm__ __volatile__("wrteei 0");
            
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
       
        /* Handle tasks's flags */
        if (task->tc_Flags & TF_EXCEPT)
            Exception();
        
        if (task->tc_Flags & TF_LAUNCH)
        {
            AROS_UFC1(void, task->tc_Launch,
                      AROS_UFCA(struct ExecBase *, SysBase, A6));       
        }
        
        /* Restore the task's state */
        bcopy(GetIntETask(task)->iet_Context, regs, sizeof(regs_t));
        /* Copy the fpu, mmx, xmm state */
#warning FIXME: Change to the lazy saving of the FPU state!!!!
#warning TODO: No FPU support yet!!!!!!! Yay, it sucks! :-D
    //    IPTR sse_ctx = ((IPTR)GetIntETask(task)->iet_Context + sizeof(regs_t) + 15) & ~15;
    //    asm volatile("fxrstor (%0)"::"D"(sse_ctx));
        
    }
    /* Leave interrupt and jump to the new task */
    core_LeaveInterrupt(regs);
}

void core_Switch(regs_t *regs)
{
    struct ExecBase *SysBase = getSysBase();
    struct Task *task;
    
    if (SysBase)
    {
        /* Disable interrupts for a while */
        __asm__ __volatile__("wrteei 0");
    
        task = SysBase->ThisTask;
        
        /* Copy current task's context into the ETask structure */
        bcopy(regs, GetIntETask(task)->iet_Context, sizeof(regs_t));
        
        /* Copy the fpu, mmx, xmm state */
#warning FIXME: Change to the lazy saving of the FPU state!!!!
#warning TODO: Write the damn FPU handling at all!!!!!!!! ;-D LOL
    //    IPTR sse_ctx = ((IPTR)GetIntETask(task)->iet_Context + sizeof(regs_t) + 15) & ~15;
    //    asm volatile("fxsave (%0)"::"D"(sse_ctx));
        
        /* store IDNestCnt into tasks's structure */  
        task->tc_IDNestCnt = SysBase->IDNestCnt;
        task->tc_SPReg = regs->gpr[1];
        
        /* And enable interrupts */
        SysBase->IDNestCnt = -1;
        __asm__ __volatile__("wrteei 1");
        
        /* TF_SWITCH flag set? Call the switch routine */
        if (task->tc_Flags & TF_SWITCH)
        {
            AROS_UFC1(void, task->tc_Switch,
                      AROS_UFCA(struct ExecBase *, SysBase, A6));
        }
    }
    
    core_Dispatch(regs);
}


/*
 * Schedule the currently running task away. Put it into the TaskReady list 
 * in some smart way. This function is subject of change and it will be probably replaced
 * by some plugin system in the future
 */
void core_Schedule(regs_t *regs)
{
    struct ExecBase *SysBase = getSysBase();
    struct Task *task;

    if (SysBase)
    {
        /* Disable interrupts for a while */
        __asm__ __volatile__("wrteei 0"); // CLI
    
        task = SysBase->ThisTask;
    
        /* Clear the pending switch flag. */
        SysBase->AttnResched &= ~ARF_AttnSwitch;
    
        /* If task has pending exception, reschedule it so that the dispatcher may handle the exception */
        if (!(task->tc_Flags & TF_EXCEPT))
        {
            /* Is the TaskReady empty? If yes, then the running task is the only one. Let it work */
            if (IsListEmpty(&SysBase->TaskReady))
                core_LeaveInterrupt(regs);         
    
            /* Does the TaskReady list contains tasks with priority equal or lower than current task?
             * If so, then check further... */
            if (((struct Task*)GetHead(&SysBase->TaskReady))->tc_Node.ln_Pri <= task->tc_Node.ln_Pri)
            {
                /* If the running task did not used it's whole quantum yet, let it work */
                if (!(SysBase->SysFlags & 0x2000))
                {
                    core_LeaveInterrupt(regs);
                }
            }
        }
    
        /* 
         * If we got here, then the rescheduling is necessary. 
         * Put the task into the TaskReady list.
         */
        task->tc_State = TS_READY;
        Enqueue(&SysBase->TaskReady, (struct Node *)task);
    }
    
    /* Select new task to run */
    core_Switch(regs);
}


/*
 * Leave the interrupt. This function recieves the register frame used to leave the supervisor
 * mode. It never returns and reschedules the task if it was asked for.
 */
void core_ExitInterrupt(regs_t *regs) 
{
    /* Going back into supervisor mode? Then exit immediatelly */
    if (!(regs->srr1 & MSR_PR))
    {
        core_LeaveInterrupt(regs);
    }
    else
    {
        /* Prepare to go back into user mode */
        struct ExecBase *SysBase = getSysBase();

        if (SysBase)
        {
            /* Soft interrupt requested? It's high time to do it */
            if (SysBase->SysFlags & SFF_SoftInt)
                core_Cause(SysBase);
    
            /* If task switching is disabled, leave immediatelly */
            if (SysBase->TDNestCnt >= 0)
            {
                core_LeaveInterrupt(regs);
            }
            else
            {
                /* 
                 * Do not disturb task if it's not necessary. 
                 * Reschedule only if switch pending flag is set. Exit otherwise.
                 */
                if (SysBase->AttnResched & ARF_AttnSwitch)
                {
                    core_Schedule(regs);
                }
                else 
                    core_LeaveInterrupt(regs);
            }
        }
        else
            core_LeaveInterrupt(regs);
    }
}

void core_Cause(struct ExecBase *SysBase)
{
    struct IntVector *iv = &SysBase->IntVects[INTB_SOFTINT];

    /* If the SoftInt vector in SysBase is set, call it. It will do the rest for us */
    if (iv->iv_Code)
    {
        AROS_UFC5(void, iv->iv_Code,
            AROS_UFCA(ULONG, 0, D1),
            AROS_UFCA(ULONG, 0, A0),
            AROS_UFCA(APTR, 0, A1),
            AROS_UFCA(APTR, iv->iv_Code, A5),
            AROS_UFCA(struct ExecBase *, SysBase, A6)
        );
    }
}
