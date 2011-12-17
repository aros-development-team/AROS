/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: High-level scheduler calling code
    Lang: English
*/

#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
 
#include <kernel_base.h>
#include <kernel_intr.h>
#include <kernel_scheduler.h>
#include <kernel_syscall.h>

/*
 * Leave the interrupt. This function receives the interrupt register frame
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
 * This routine dispatches scheduler's SysCall, when some task wants to give up
 * the CPU time explicitly.
 * Similar to above, it should be called only when you're returning to user mode.
 */
void core_SysCall(int sc, regs_t *regs)
{
    switch (sc)
    {
    case SC_CAUSE:
	core_ExitInterrupt(regs);
        break;

    case SC_SCHEDULE:
        if (!core_Schedule())
            break;
        /* Fallthrough */

    case SC_SWITCH:
        cpu_Switch(regs);
        /* Fallthrough */

    case SC_DISPATCH:
        cpu_Dispatch(regs);
        break;
    }
}
