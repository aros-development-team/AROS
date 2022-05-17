/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.

    Desc: Send some signal to a given task
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#define __AROS_KERNEL__
#include "exec_intern.h"

#if defined(__AROSEXEC_SMP__)
#include <utility/hooks.h>

#include "kernel_ipi.h"

AROS_UFH3(IPTR, signal_hook,
    AROS_UFHA(struct IPIHook *, hook, A0),
    AROS_UFHA(APTR, object, A2),
    AROS_UFHA(APTR, message, A1)
)
{
    AROS_USERFUNC_INIT

    struct ExecBase *SysBase = (struct ExecBase *)hook->ih_Args[0];
    struct Task *target = (struct Task *)hook->ih_Args[1];
    ULONG sigset = (ULONG)hook->ih_Args[2];

    D(
        struct KernelBase *KernelBase = __kernelBase;
        int cpunum = KrnGetCPUNumber();
        bug("[Exec] CPU%03d: Using IPI to do Signal(%p, %08x), SysBase=%p\n", cpunum, msg->target, msg->sigset, SysBase);
    );
    
    Signal(target, sigset);

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
    struct KernelBase *KernelBase = __kernelBase;
    int cpunum = KrnGetCPUNumber();

    /*
        * # If current CPU number is not the task's CPU and the task is running now, send signal to that task
        *   from CPU which the task is running on.
        * # If task is not running and the current CPU is not in the Affinitymask, send signal to CPU form Affinity mask
        * # If task is not running and the current CPU is in the Affinity mask, just proceed with regular signal
    */
    if ((PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) &&
        ((IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber != cpunum && task->tc_State == TS_RUN) ||
        !KrnCPUInMask(cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity)))
    {
        struct Hook h;
        IPTR args[3];
        // We cannot use KrnAllocCPUMask() since this function uses AllocMem
        // And we cannot use AllocMem from interrupts (where Signal() is allowed)...
        ULONG mask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // CPU mask large enough for 256 CPUs...
        void *cpu_mask = &mask;

        args[0] = (IPTR)SysBase;
        args[1] = (IPTR)task;
        args[2] = (IPTR)signalSet;

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

        D(bug("[Exec] %s: Signaling from CPU%03d -> CPU%03d using IPI...\n", __func__, cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber));

        h.h_Entry = signal_hook;

        D(bug("[Exec] %s: Sending IPI...\n", __func__));
        core_DoCallIPI(&h, cpu_mask, 1, 3, args, (void *)KernelBase);
        D(bug("[Exec] %s: IPI Sent\n", __func__));
    }
    else
    {

        if (cpunum != 0)
        {
            D(bug("[Exec] %s(0x%p, %08lX) on CPU%03d\n", __func__, task, signalSet, cpunum));
        }
#else
        D(bug("[Exec] %s(0x%p, %08lX)\n", __func__, task, signalSet);)
#endif

        Disable();
        D(
            bug("[Exec] %s: Signaling 0x%p '%s', state = %08x\n", __func__, task, task->tc_Node.ln_Name, task->tc_State);
#if (0)
            if (((struct KernelBase *)KernelBase)->kb_ICFlags & KERNBASEF_IRQPROCESSING)
                bug("[Exec] Signal: (Called from Interrupt)\n");
            else
#endif
                bug("[Exec] %s: (Called from '%s')\n", __func__, thisTask->tc_Node.ln_Name);
        )

        D(bug("[Exec] %s: Target signal flags : %08x ->", __func__, task->tc_SigRecvd);)
        /* Set the signals in the task structure. */
#if defined(__AROSEXEC_SMP__)
        __AROS_ATOMIC_OR_L(task->tc_SigRecvd, signalSet);
#else
        task->tc_SigRecvd |= signalSet;
#endif
        D(bug(" %08x\n", task->tc_SigRecvd);)

        /* Do those bits raise exceptions? */
        if (task->tc_SigRecvd & task->tc_SigExcept)
        {
            /* Yes. Set the exception flag. */
#if defined(__AROSEXEC_SMP__)
            __AROS_ATOMIC_OR_B(task->tc_Flags, TF_EXCEPT);
#else
            task->tc_Flags |= TF_EXCEPT;
#endif
            D(bug("[Exec] %s: TF_EXCEPT set\n", __func__);)

            /*
                    if the target task is running (called from within interrupt handler, or from another processor),
                    raise the exception or defer it for later.
                */
            if (task->tc_State == TS_RUN)
            {
#if defined(__AROSEXEC_SMP__)
                if (!(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) ||
                    (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber == cpunum))
                {
#endif
                D(bug("[Exec] %s: calling Reschedule to raise Exception for RUN Task\n", __func__);)
                /* Order a reschedule */
                Reschedule();
#if defined(__AROSEXEC_SMP__)
                }
                else
                {
                    D(bug("[Exec] %s: Raising Exception for RUN Task on CPU %03u\n", __func__, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber));
                    KrnScheduleCPU(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
                }
#endif
                Enable();

                /* All done. */
                return;
            }
        }

        /* Does the target task have signals to process ? */
        if (task->tc_SigRecvd & (task->tc_SigWait | task->tc_SigExcept))
        {
            if (task->tc_State == TS_WAIT)
            {
                D(
                bug("[Exec] %s: Signaling WAIT Task 0x%p '%s' pri %d\n", __func__, task, task->tc_Node.ln_Name, task->tc_Node.ln_Pri);
                )
                /* Yes. Move it to the ready list. */
#if defined(__AROSEXEC_SMP__)
                krnSysCallReschedTask(task, TS_READY);
#else
                Remove(&task->tc_Node);
                task->tc_State = TS_READY;
                Enqueue(&SysBase->TaskReady, &task->tc_Node);
#endif
            }

            if (task->tc_State == TS_READY)
            {
                /* Has it a higher priority than the running task? */
                if (
#if defined(__AROSEXEC_SMP__)
                    (!(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) ||
                    (KrnCPUInMask(cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity))) &&
#endif
                    (task->tc_Node.ln_Pri > thisTask->tc_Node.ln_Pri))
                {
                    D(bug("[Exec] %s: Task has higher priority ...\n", __func__);)
                    /*
                        Yes. A taskswitch is necessary. Prepare one if possible.
                        (If the current task is not running it is already moved)
                    */
                    if (thisTask->tc_State == TS_RUN)
                    {
                        D(
                            bug("[Exec] %s: Rescheduling RUN Task 0x%p '%s' pri %d,  to let 0x%p '%s' process the signal...\n", __func__, thisTask, thisTask->tc_Node.ln_Name, thisTask->tc_Node.ln_Pri, task, task->tc_Node.ln_Name);
                        )
                        Reschedule();
                        D(
                            bug("[Exec] %s: returned to task 0x%p '%s' pri %d\n", __func__, thisTask, thisTask->tc_Node.ln_Name, thisTask->tc_Node.ln_Pri);
                        )
                    }
                }
#if defined(__AROSEXEC_SMP__)
                else if ((PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) &&
                    !(KrnCPUInMask(cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity)))
                {
                    D(bug("[Exec] %s: Signaling task on another CPU\n", __func__));
                    krnSysCallReschedTask(task, TS_READY);
                    KrnScheduleCPU(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
                }
#endif
                D(else bug("[Exec] %s: Letting Task process signal when next scheduled to run...\n", __func__);)
            }
        }

        Enable();

        D(
            bug("[Exec] %s: 0x%p finished signal processing\n", __func__, task);
        )
#if defined(__AROSEXEC_SMP__)
    }
#endif

    AROS_LIBFUNC_EXIT
}
