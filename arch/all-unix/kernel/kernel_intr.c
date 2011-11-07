#include <aros/atomic.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
 
#include <kernel_base.h>
#include <kernel_interrupts.h>
#include <kernel_intr.h>
#include <kernel_globals.h>
#include <kernel_scheduler.h>
#include <kernel_syscall.h>
#include "kernel_intern.h"
#include "kernel_unix.h"

/*
 * Leave the interrupt. This function recieves the interrupt register frame
 * and runs task scheduler if needed.
 *
 * You should call it only if you're returning to user mode (i.e. not from
 * within nested interrupt).
 *
 * It relies on CPU-specific cpu_Switch() and cpu_Dispatch() implementations
 * which save and restore CPU context. cpu_Dispatch() is allowed just to
 * jump to the saved context and not return here.
 */
void core_ExitInterrupt(regs_t *regs) 
{
    /* Soft interrupt requested? It's high time to do it */
    if (SysBase->SysFlags & SFF_SoftInt)
        core_Cause(INTB_SOFTINT, 1L << INTB_SOFTINT);

    /* If task switching is disabled, do nothing */
    if (SysBase->TDNestCnt < 0)
    {
        /*
         * Do not disturb task if it's not necessary. 
         * Reschedule only if switch pending flag is set. Exit otherwise.
         */
        if (SysBase->AttnResched & ARF_AttnSwitch)
        {
	    /* Run task scheduling sequence */
            if (core_Schedule())
	    {
		cpu_Switch(regs);
		cpu_Dispatch(regs);
            }
	}
    }
}

/*
 * We do not have enough user-defined signals to map
 * them to four required syscalls (CAUSE, SCHEDULE,
 * SWITCH, DISPATCH). We get around this by assigning
 * CAUSE to SIGUSR2 and others to SIGUSR1.
 *
 * What action is to be taken upon SIGUSR1 can be
 * figured out by looking at caller task's state.
 * exec.library calls KrnDispatch() only after task
 * has been actually disposed, with state set to TS_REMOVED.
 * Similarly, KrnSwitch() is called only in Wait(), after
 * setting state to TS_WAIT. In other cases it's KrnSchedule().
 */
void core_SysCall(int sig, regs_t *regs)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct Task *task = SysBase->ThisTask;

    AROS_ATOMIC_INC(UKB(KernelBase)->SupervisorCount);

    krnRunIRQHandlers(KernelBase, sig);

    switch(task->tc_State)
    {
    /* A running task needs to be put into TaskReady list first. It's SC_SCHEDULE. */
    case TS_RUN:
	if (!core_Schedule())
	    break;

    /* If the task is already in some list with appropriate state, it's SC_SWITCH */
    case TS_READY:
    case TS_WAIT:
	cpu_Switch(regs);

    /* If the task is removed, it's simply SC_DISPATCH */
    case TS_REMOVED:
	cpu_Dispatch(regs);
	break;

    /* Special state is used for returning from exception */
    case TS_EXCEPT:
	cpu_DispatchContext(task, regs, KernelBase->kb_PlatformData);
	break;
    }

    AROS_ATOMIC_DEC(UKB(KernelBase)->SupervisorCount);
}
