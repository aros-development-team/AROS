/*
    Copyright (C) 2011-2020, The AROS Development Team. All rights reserved.

    Desc: High-level scheduler calling code
*/

#include <aros/atomic.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
 
#include <kernel_base.h>
#include <proto/kernel.h>
#include <kernel_debug.h>
#include <kernel_intr.h>
#include <kernel_scheduler.h>
#include <kernel_syscall.h>
#include <kernel_ipicall.h>

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
#if defined(__AROSEXEC_SMP__) && defined(FLAG_SCHEDEVICT_SET)
#include <proto/kernel.h>
#include <aros/types/spinlock_s.h>
#include "etask.h"
#include "exec_intern.h"

/*
 * RemTask() on another CPU wants the task this CPU runs. The reschedule
 * entry has marked it (TS_REMOVED and iet_Evict, under the TaskRunning
 * lock) and left it on TaskRunning, because only this CPU can take it off
 * itself. This is called where the CPU is about to return to the task, and
 * does not return to it: the task's Forbid() and Disable() counts do not
 * count, it is not coming back.
 *
 * cpu_Switch() takes it off TaskRunning (TS_TOMBSTONED). The CPU is on its
 * own stack, as on every SMP port in kernel context, so from the moment
 * TS_INVALID is published nothing of the task is in use here and the
 * RemTask() that waits for it may free everything. The scheduler is told
 * (FLAG_SCHEDEVICT) not to look at the task again.
 *
 * A task that removes itself is TS_REMOVED too, for a moment, but has no
 * iet_Evict: it has its RemTask() to finish.
 */
static BOOL core_EvictTask(regs_t *regs)
{
    struct Task *task;
    struct IntETask *iet;

    if (FLAG_SCHEDEVICT_ISSET)
        return FALSE;

    task = GET_THIS_TASK;
    if (!task || (__atomic_load_n(&task->tc_State, __ATOMIC_ACQUIRE) != TS_REMOVED) ||
        !(task->tc_Flags & TF_ETASK) || !(iet = IntETask(task->tc_UnionETask.tc_ETask)) ||
        !iet->iet_Evict)
        return FALSE;

    cpu_Switch(regs);
    /*
     * The CPU's Forbid() count was the task's, and goes with it; the next
     * task would inherit it otherwise. (The Disable() count is set from
     * the next task when it is dispatched.)
     */
    TDNESTCOUNT_SET(-1);
    FLAG_SCHEDEVICT_SET;
    __atomic_store_n(&task->tc_State, TS_INVALID, __ATOMIC_RELEASE);
    cpu_Dispatch(regs);

    return TRUE;
}
#else
#define core_EvictTask(regs) (FALSE)
#endif

/*
 * Where a CPU ends when it cannot go on (a fault in supervisor code). It
 * runs no task again, but it does not go deaf: it keeps running the calls
 * the other CPUs send it, so none of them waits for it for ever, and it
 * lets go of the task it had if another CPU's RemTask() asks for it.
 *
 * This is best effort: the CPU got here because something is wrong with
 * it, and it may have been holding a lock. The run list lock is only
 * tried, never waited for.
 */
void core_HaltCPU(void)
{
    for (;;)
    {
#if defined(__AROSEXEC_SMP__) && defined(FLAG_SCHEDEVICT_SET)
        struct Task *task = SysBase ? GET_THIS_TASK : NULL;
        struct IntETask *iet;

        if (task && !FLAG_SCHEDEVICT_ISSET &&
            (__atomic_load_n(&task->tc_State, __ATOMIC_ACQUIRE) == TS_REMOVED) &&
            (task->tc_Flags & TF_ETASK) && (iet = IntETask(task->tc_UnionETask.tc_ETask)) &&
            iet->iet_Evict &&
            KrnSpinTryLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, SPINLOCK_MODE_WRITE))
        {
            /* Nothing of the task is used here any more: this CPU runs no task again */
            REMOVE(&task->tc_Node);
            task->tc_State = TS_TOMBSTONED;
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);

            FLAG_SCHEDEVICT_SET;
            __atomic_store_n(&task->tc_State, TS_INVALID, __ATOMIC_RELEASE);
        }
#endif
#if defined(KRN_IPICALL)
        krnIPICallDrain();
        krnIPIPortPause();
#endif
    }
}

void core_ExitInterrupt(regs_t *regs)
{
    if (core_EvictTask(regs))
        return;

    /* Soft interrupt requested? It's high time to do it */
    if (SysBase->SysFlags & SFF_SoftInt)
    {
        core_Cause(INTB_SOFTINT, 1L << INTB_SOFTINT);
        /* SoftIntDispatch() returns with interrupts on; scheduling needs them off */
        KrnCli();
    }

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
    /* Whatever a task that is being removed asked for, it is not done */
    if (core_EvictTask(regs))
        return;

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

#if defined(__AROSEXEC_SMP__) && defined(KRNSYSCALL_ARG1)
    /*
     * The run list moves exec asks for. They are made here, in kernel
     * context with interrupts off, under the run list locks. Ports whose
     * kernel_cpu.h says where a system call's arguments are use these;
     * all-pc has its own handlers (arch/all-pc/exec/platform_init.c).
     */
    case SC_RESCHEDTASK:
        core_ReschedTask((struct Task *)KRNSYSCALL_ARG1(regs), (ULONG)KRNSYSCALL_ARG2(regs));
        break;

    case SC_REMSWITCH:
        cpu_Switch(regs);
        break;
#endif
    }
}
