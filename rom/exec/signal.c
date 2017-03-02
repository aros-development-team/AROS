/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Send some signal to a given task
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"

#if defined(__AROSEXEC_SMP__)
#define __KERNEL_NOLIBBASE__
#include <aros/types/spinlock_s.h>
#include <proto/kernel.h>
#include <utility/hooks.h>

void core_DoCallIPI(struct Hook *hook, void *cpu_mask, int async, APTR *_KernelBase);

struct signal_message {
    struct ExecBase *   SysBase;
    struct Task *       target;
    ULONG               sigset;
};

AROS_UFH3(IPTR, signal_hook,
    AROS_UFHA(struct Hook *, hook, A0), 
    AROS_UFHA(APTR, object, A2), 
    AROS_UFHA(APTR, message, A1)
)
{
    AROS_USERFUNC_INIT

    struct signal_message *msg = hook->h_Data;
    struct ExecBase *SysBase = msg->SysBase;

    D(
        int cpunum = KrnGetCPUNumber();
        bug("[Exec] CPU%03d: Using IPI to do Signal(%p, %08x), SysBase=%p\n", cpunum, msg->target, msg->sigset, SysBase);
    );
    
    Signal(msg->target, msg->sigset);

    return 0;

    AROS_USERFUNC_EXIT
}
#endif

/*****************************************************************************

    NAME */

        AROS_LH2(void, Signal,

/*  SYNOPSIS */
        AROS_LHA(struct Task *,     task,      A1),
        AROS_LHA(ULONG,             signalSet, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 54, Exec)

/*  FUNCTION
        Send some signals to a given task. If the task is currently waiting
        on these signals, has a higher priority as the current one and if
        taskswitches are allowed the new task begins to run immediately.

    INPUTS
        task      - Pointer to task structure.
        signalSet - The set of signals to send to the task.

    RESULT

    NOTES
        This function may be used from interrupts.

    EXAMPLE

    BUGS

    SEE ALSO
        AllocSignal(), FreeSignal(), Wait(), SetSignal(), SetExcept()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *thisTask = GET_THIS_TASK;
#if defined(__AROSEXEC_SMP__)
    int cpunum = KrnGetCPUNumber();

    //EXEC_SPINLOCK_LOCK(IntETask(task->tc_UnionETask.tc_ETask)->iet_SpinLock, NULL, SPINLOCK_MODE_READ);
    /*
        * If current CPU number is not the task's CPU and the task is running now, send signal to that task
        from CPU which the task is running on
        * If task is not running and the current CPU is not in the Affinitymask, send signal to CPU form Affinity mask
        * If task is not running and the current CPU is in the Affinity mask, just proceed with regular signal
    */
    if ((IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber != cpunum && task->tc_State == TS_RUN) ||
        !KrnCPUInMask(cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity))
    {
        struct Hook h;
        struct signal_message msg;
        void *cpu_mask = KrnAllocCPUMask();

        /* Task is running *now* on another CPU, send signal there */
        if (task->tc_State == TS_RUN)
        {
            KrnGetCPUMask(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber, cpu_mask);
        }
        else
        {
            int i;
            int cpumax = KrnGetCPUCount();

            /* Task is not running now, find first cpu suitable to run this task. Use CPU balancing some day... */
            for (i=0; i < cpumax; i++)
            {
                if (KrnCPUInMask(i, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity))
                {
                    KrnGetCPUMask(i, cpu_mask);
                    break;
                }
            }
        }

        msg.SysBase = SysBase;
        msg.target = task;
        msg.sigset = signalSet;

        D(bug("[Exec] Signal goes from CPU%03d to CPU%03d. calling Signal on that cpu with IPI...\n", cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber));

        h.h_Entry = signal_hook;
        h.h_Data = &msg;

        D(bug("[Exec] Sending IPI...\n"));
        core_DoCallIPI(&h, cpu_mask, 0, KernelBase);
        D(bug("[Exec] IPI Sent\n"));
        
        KrnFreeCPUMask(cpu_mask);
    }
    //EXEC_SPINLOCK_UNLOCK(IntETask(task->tc_UnionETask.tc_ETask)->iet_SpinLock);

    if (cpunum != 0)
    {
        D(bug("[Exec] Signal(0x%p, %08lX) on CPU%03d\n", task, signalSet, cpunum));
        D(bug("[Exec] Signal: signaling '%s' (state %08x)\n", task->tc_Node.ln_Name, task->tc_State));
    }
#endif

    D(
        bug("[Exec] Signal(0x%p, %08lX)\n", task, signalSet);
        bug("[Exec] Signal: signaling '%s' (state %08x)\n", task->tc_Node.ln_Name, task->tc_State);
        bug("[Exec] Signal: from '%s'\n", thisTask->tc_Node.ln_Name);
    )

    Disable();
    /* Set the signals in the task structure. */
#if defined(__AROSEXEC_SMP__)
    __AROS_ATOMIC_OR_L(task->tc_SigRecvd, signalSet);
#else
    task->tc_SigRecvd |= signalSet;
#endif

    /* Do those bits raise exceptions? */
    if (task->tc_SigRecvd & task->tc_SigExcept)
    {
        /* Yes. Set the exception flag. */
#if defined(__AROSEXEC_SMP__)
        __AROS_ATOMIC_OR_B(task->tc_Flags, TF_EXCEPT);
#else
        task->tc_Flags |= TF_EXCEPT;
#endif

        D(bug("[Exec] Signal: TF_EXCEPT set\n");)
    }

    /* 
            if the target task is running (called from within interrupt handler),
            raise the exception or defer it for later.
        */
    if ((task->tc_SigRecvd & task->tc_SigExcept) &&
        (task->tc_State == TS_RUN))
    {
#if defined(__AROSEXEC_SMP__)
        if (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber == cpunum)
        {
#endif
        D(bug("[Exec] Signal: signaling running task\n");)
        /* Order a reschedule */
        Reschedule();
#if defined(__AROSEXEC_SMP__)
        }
        else
        {
            D(bug("[Exec] Signal: raising exception in task on another cpu (%03u)\n", IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber));
            KrnScheduleCPU(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
        }
#endif
        Enable();

        /* All done. */
        return;
    }

    /*
        Is the task receiving the signals waiting on them
        (or on a exception) ?
    */
    if ((task->tc_State == TS_WAIT) &&
       (task->tc_SigRecvd & (task->tc_SigWait | task->tc_SigExcept)))
    {
        D(bug("[Exec] Signal: signaling waiting task\n");)

        /* Yes. Move it to the ready list. */
#if defined(__AROSEXEC_SMP__)
        krnSysCallReschedTask(task, TS_READY);
#else
        Remove(&task->tc_Node);
        task->tc_State = TS_READY;
        Enqueue(&SysBase->TaskReady, &task->tc_Node);
#endif
        /* Has it a higher priority as the current one? */
        if (
#if defined(__AROSEXEC_SMP__)
            (!(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) ||
            (KrnCPUInMask(cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity))) &&
#endif
            (task->tc_Node.ln_Pri > thisTask->tc_Node.ln_Pri))
        {
            /*
                Yes. A taskswitch is necessary. Prepare one if possible.
                (If the current task is not running it is already moved)
            */
            if (thisTask->tc_State == TS_RUN)
            {
                Reschedule();
            }
        }
#if defined(__AROSEXEC_SMP__)
        else if ((PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) &&
            !(KrnCPUInMask(cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity)))
        {
            D(bug("[Exec] Signal: signaling task on another CPU\n"));
            krnSysCallReschedTask(task, TS_READY);
            KrnScheduleCPU(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
        }
#endif
    }

    Enable();

    D(bug("[Exec] Signal: 0x%p finished signal processing\n", task);)

    AROS_LIBFUNC_EXIT
}

