/*
    Copyright (C) 2011-2020, The AROS Development Team. All rights reserved.

    Desc: High-level scheduler calling code
*/

#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
 
#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_intr.h>
#include <kernel_scheduler.h>
#include <kernel_syscall.h>

#if defined(__AROSEXEC_SMP__)
#include "exec_platform.h"
#endif

#define D(x)

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
    if (TDNESTCOUNT_GET < 0)
    {
        /*
         * Do not disturb task if it's not necessary.
         * Reschedule only if switch pending flag is set. Exit otherwise.
         */
        if (FLAG_SCHEDSWITCH_ISSET)
        {
            D(bug("[Kernel] %s: Triggering Schedule\n", __func__);)
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
#if defined(__AROSEXEC_SMP__)
        {
            /*
             * Only a task that removes itself dispatches (RemTask). It has
             * left its stack with this trap, which every SMP port takes on
             * the CPU's own stack: only now may the service task, perhaps
             * on another CPU, free that stack (it does once TS_INVALID).
             * Not when SC_SWITCH falls through: a finalising task switched
             * away still has its RemTask to finish.
             */
            struct Task *task = GET_THIS_TASK;

            if ((sc == SC_DISPATCH) && task && (task->tc_State == TS_TOMBSTONED))
                __atomic_store_n(&task->tc_State, TS_INVALID, __ATOMIC_RELEASE);
        }
#endif
        cpu_Dispatch(regs);
        break;
    }
}
