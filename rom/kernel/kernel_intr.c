/*
 * This code is an attempt to merge CPU-specific Dispatch() parts.
 * This is expected to be suitable for most architectures.
 *
 * The following functions need to be implemented:
 * - void core_Exception(regs_t *regs) - set up exception on the context;
 * - void core_LeaveInterrupt(regs_t *regs) - jump to context in regs.
 *
 * This code is completely untested.
 */

#include "../exec/etask.h"
 
#include <kernel_cpu.h>
#include <kernel_scheduler.h>

/*** The following macros can be defined in kernel_cpu.h ***/

/* Restore CPU context from ctx to regs */
#ifndef RESTOREREGS(ctx, regs)
#define RESTOREREGS(ctx, regs) regs = ctx
#endif

/* Adjust interrupt state in the context */
#ifndef REGS_STI
#define REGS_STI(state, regs)
#endif

/* We must have either HALT definition or idle task */
#ifndef HALT
#define HALT bug("[KRN] Kernel panic! No running tasks left!\n"); for (;;)
#endif

/*
 * Generic dispatcher with registers restore and sleep mode handling.
 * Fits for most of architectures.
 */
void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;

    while (!(task = core_Dispatch()))
    {
	/* 
	 * Is the list of ready tasks empty? Well, increment the idle switch cound and halt CPU.
	 * It should be extended by some plugin mechanism which would put CPU and whole machine
	 * into some more sophisticated sleep states (ACPI?)
	 */
        SysBase->IdleCount++;
        SysBase->AttnResched |= ARF_AttnSwitch;

        /* Sleep almost forever ;) */
        HALT;

        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT);
    }

    RESTOREREGS(GetIntETask(task)->iet_Context, regs);
#ifdef REGS_NO_SP
    SP(regs) = (IPTR)task->tc_SPReg;
#endif

    STI(SysBase->IDNestCnt < 0, regs);

    if (task->tc_Flags & TF_EXCEPT)
	core_Exception(regs);

    core_LeaveInterrupt(regs);
}

/*
 * Leave the interrupt. This function recieves the register frame used to leave the supervisor
 * mode. It never returns and reschedules the task if it was asked for.
 */
void core_ExitInterrupt(regs_t *regs) 
{
    /* Powermode was on? Turn it off now */
    SLEEP_OFF(regs);

    /* Going back into supervisor mode? Then do nothing */
    if (IS_USER(regs))
    {
        /* Soft interrupt requested? It's high time to do it */
        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT);

        /* If task switching is disabled, do nothing */
        if (SysBase->TDNestCnt < 0)
        {
            /*
             * Do not disturb task if it's not necessary. 
             * Reschedule only if switch pending flag is set. Exit otherwise.
             */
            if (SysBase->AttnResched & ARF_AttnSwitch)
            {
                if (core_Schedule())
		{


		    core_Switch();
		    cpu_Dispatch(regs);
		}
            }
	}
    }

    core_LeaveInterrupt(regs);
}
