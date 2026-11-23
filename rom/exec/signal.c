/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

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
#include <aros/atomic.h>
#include <utility/hooks.h>

#include "etask.h"
#include "kernel_ipi.h"

/*
 * A task belongs to one CPU at a time, and its state is changed on that
 * CPU. A Signal() for a task that is running on another CPU, or that this
 * CPU may not run, is not carried out here: it is sent to a CPU that owns
 * the task and carried out there, by this hook.
 */
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
        bug("[Exec] CPU%03d: Using IPI to do Signal(%p, %08x), SysBase=%p\n", cpunum, target, sigset, SysBase);
    );

    Signal(target, sigset);

    return 0;

    AROS_USERFUNC_EXIT
}

/*
 * Hand Signal(task, signalSet) to a CPU that owns the task. FALSE if that
 * CPU turns out to be this one, and the caller has to carry it out.
 */
static BOOL signal_Forward(struct Task *task, ULONG signalSet, ULONG state, int cpunum, struct ExecBase *SysBase)
{
    struct KernelBase *KernelBase = __kernelBase;
    struct IntETask *iet = IntETask(task->tc_UnionETask.tc_ETask);
    struct Hook h;
    IPTR args[3];
    // We cannot use KrnAllocCPUMask() since this function uses AllocMem
    // And we cannot use AllocMem from interrupts (where Signal() is allowed)...
    ULONG mask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; // CPU mask large enough for 256 CPUs...
    void *cpu_mask = &mask;
    int targetcpu = 0;

    args[0] = (IPTR)SysBase;
    args[1] = (IPTR)task;
    args[2] = (IPTR)signalSet;

    /* Task is running *now* on another CPU, send signal there */
    if (state == TS_RUN)
    {
        targetcpu = iet->iet_CpuNumber;
    }
    else
    {
        int i;
        int cpumax = KrnGetCPUCount();

        /* Task is not running now, find first cpu suitable to run this task. Use CPU balancing some day... */
        for (i=0; i < cpumax; i++)
        {
            if (KrnCPUInMask(i, iet->iet_CpuAffinity))
            {
                targetcpu = i;
                break;
            }
        }
    }

    if (targetcpu == cpunum)
        return FALSE;

    D(bug("[Exec] %s: Signaling CPU%03d using IPI...\n", __func__, targetcpu));

    h.h_Entry = signal_hook;
    KrnGetCPUMask(targetcpu, cpu_mask);
    core_DoCallIPI(&h, cpu_mask, 1, 3, args, (void *)KernelBase);

    return TRUE;
}

/*
 * Called from RemTask() once the task is terminal, before its memory can
 * go away, so no CPU carries out a queued Signal() naming a freed task.
 */
void Exec_CancelSignalIPIs(struct Task *task, struct ExecBase *SysBase)
{
    core_CancelCallIPIs((APTR)signal_hook, (IPTR)task);
}
#endif

/*****************************************************************************

    NAME */

#if defined(__mc68000)
void Exec_SignalSlow(struct Task *task, ULONG signalSet, struct ExecBase *SysBase)
#else
        AROS_LH2(void, Signal,

/*  SYNOPSIS */
        AROS_LHA(struct Task *,     task,      A1),
        AROS_LHA(ULONG,             signalSet, D0),

/*  LOCATION */
        struct ExecBase *, SysBase, 54, Exec)
#endif

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
#if !defined(__mc68000)
    AROS_LIBFUNC_INIT
#endif

    struct Task *thisTask = GET_THIS_TASK;
    ULONG state;

#if defined(__AROSEXEC_SMP__)
    struct KernelBase *KernelBase = __kernelBase;
    int cpunum = KrnGetCPUNumber();

    /*
        * # If current CPU number is not the task's CPU and the task is running now, send signal to that task
        *   from CPU which the task is running on.
        * # If task is not running and the current CPU is not in the Affinitymask, send signal to CPU form Affinity mask
        * # If task is not running and the current CPU is in the Affinity mask, just proceed with regular signal
    */
    /*
     * The scheduler writes the task's CPU number before it publishes
     * TS_RUN; the state is read first here, with acquire ordering, so the
     * number read after it is the right one.
     */
    state = __atomic_load_n(&task->tc_State, __ATOMIC_ACQUIRE);

    /*
     * A task that is being removed is on no run list and is not woken. Its
     * signal is recorded and nothing is sent after it: RemTask() drops the
     * calls queued for the task once it is in this state, and must not
     * find a new one behind its back.
     */
    if ((state == TS_REMOVED) || (state == TS_TOMBSTONED) || (state == TS_INVALID))
    {
        __AROS_ATOMIC_OR_L(task->tc_SigRecvd, signalSet);
        return;
    }

    if ((PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) &&
        (((state == TS_RUN) && (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber != cpunum)) ||
        !KrnCPUInMask(cpunum, IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuAffinity)))
    {
        /*
         * Signals are not counted. If every one of these is pending for
         * the task already, whoever set them has woken the task or is
         * about to, and the task cannot wait again without seeing them;
         * there is nothing to add, and nothing to send. Without this a
         * task that many CPUs signal keeps its own CPU in the IPI handler.
         */
        if ((task->tc_SigRecvd & signalSet) == signalSet)
            return;

        if (signal_Forward(task, signalSet, state, cpunum, SysBase))
            return;
    }

    if (cpunum != 0)
    {
        D(bug("[Exec] %s(0x%p, %08lX) on CPU%03d\n", __func__, task, signalSet, cpunum));
    }
#else
    D(bug("[Exec] %s(0x%p, %08lX)\n", __func__, task, signalSet);)
#endif

    Disable();

    D(bug("[Exec] %s: Target signal flags : %08x ->", __func__, task->tc_SigRecvd);)
    /* Set the signals in the task structure. */
#if defined(__AROSEXEC_SMP__)
    __AROS_ATOMIC_OR_L(task->tc_SigRecvd, signalSet);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);

    /*
     * The state is read after the bits are set, and only once. The task
     * can have been taken off TaskReady and started on another CPU since
     * the test above. It then belongs to that CPU: it may be about to test
     * its signals and wait, and only that CPU can tell. Send the signal
     * there, as the test above does for a task that was already running.
     */
    state = __atomic_load_n(&task->tc_State, __ATOMIC_ACQUIRE);
    if ((state == TS_RUN) &&
        (PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) &&
        (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber != cpunum))
    {
        if (signal_Forward(task, signalSet, state, cpunum, SysBase))
        {
            Enable();
            return;
        }
    }
#else
    task->tc_SigRecvd |= signalSet;
    state = task->tc_State;
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
        if (state == TS_RUN)
        {
            D(bug("[Exec] %s: calling Reschedule to raise Exception for RUN Task\n", __func__);)
            /* Order a reschedule */
            Reschedule();
            Enable();

            /* All done. */
            return;
        }
    }

    /* Does the target task have signals to process ? */
    if (task->tc_SigRecvd & (task->tc_SigWait | task->tc_SigExcept))
    {
#if defined(__AROSEXEC_SMP__)
        if ((state == TS_WAIT) || (state == TS_READY))
        {
            /*
             * Once the task is READY another CPU can run it, and it can be
             * gone before we look again. Read what is needed first and do
             * not touch the task afterwards.
             */
            BYTE targetPri = task->tc_Node.ln_Pri;

            if (state == TS_WAIT)
            {
                D(bug("[Exec] %s: Signaling WAIT Task 0x%p\n", __func__, task);)
                /*
                 * Move it to the ready list. The move is made in kernel
                 * context under the list locks, and it looks at the state
                 * again there: if another CPU has woken the task already,
                 * nothing is done.
                 */
                krnSysCallReschedTask(task, TS_READY);
            }

            /* Has it a higher priority than the running task? */
            if ((targetPri > thisTask->tc_Node.ln_Pri) && (thisTask->tc_State == TS_RUN))
            {
                D(bug("[Exec] %s: Task has higher priority ...\n", __func__);)
                /*
                    Yes. A taskswitch is necessary. Prepare one if possible.
                    (If the current task is not running it is already moved)
                */
                Reschedule();
            }
        }
#else
        if (state == TS_WAIT)
        {
            D(
            bug("[Exec] %s: Signaling WAIT Task 0x%p '%s' pri %d\n", __func__, task, task->tc_Node.ln_Name, task->tc_Node.ln_Pri);
            )
            /* Yes. Move it to the ready list. */
            Remove(&task->tc_Node);
            task->tc_State = TS_READY;
            Enqueue(&SysBase->TaskReady, &task->tc_Node);
        }

        if (task->tc_State == TS_READY)
        {
            /* Has it a higher priority than the running task? */
            if (task->tc_Node.ln_Pri > thisTask->tc_Node.ln_Pri)
            {
                D(bug("[Exec] %s: Task has higher priority ...\n", __func__);)
                /*
                    Yes. A taskswitch is necessary. Prepare one if possible.
                    (If the current task is not running it is already moved)
                */
                if (thisTask->tc_State == TS_RUN)
                {
                    Reschedule();
                }
            }
            D(else bug("[Exec] %s: Letting Task process signal when next scheduled to run...\n", __func__);)
        }
#endif
    }

    Enable();

    D(
        bug("[Exec] %s: 0x%p finished signal processing\n", __func__, task);
    )

#if !defined(__mc68000)
    AROS_LIBFUNC_EXIT
#endif
}
