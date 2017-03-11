/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: exec.library's internal service task. Performs memory management according
          to task scheduler's requests.
    Lang: english
*/

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "exec_debug.h"

void ServiceTask(struct ExecBase *SysBase)
{
#if defined(__AROSEXEC_SMP__)
    struct Task *serviceTask;
#endif
    struct Task *task;
    struct MemList *mb, *mbnext;
    BOOL taskRequeue;
    DINIT("ServiceTask: Started up");

#if defined(__AROSEXEC_SMP__)
    serviceTask = FindTask(NULL);
    DINIT("ServiceTask: task @ 0x%p", serviceTask);
#endif

    do
    { /* forever */


        while ((task = (struct Task *)GetMsg(PrivExecBase(SysBase)->ServicePort)))
        {
            DREMTASK("ServiceTask: Request for Task 0x%p, State %08X\n", task, task->tc_State);
            taskRequeue = TRUE;

            /*
             * If we ever need to use TSS here, we'll need to explicitly check its size here.
             * However we don't need this, because we never call so high-level libraries.
             * So, currently we ignore this.
             */

	    switch (task->tc_State)
	    {
#if defined(__AROSEXEC_SMP__)
            case TS_TOMBSTONED:
                if (!(PrivExecBase(SysBase)->IntFlags & EXECF_CPUAffinity) || (GetIntETask(serviceTask)->iet_CpuNumber== (IPTR)task->tc_UserData))
                {
                    DREMTASK("ServiceTask: Task is running on this CPU (%03u)\n", task->tc_UserData);
                    taskRequeue = FALSE;
                }
#endif
            case TS_REMOVED:
                if (taskRequeue)
                {
                    DREMTASK("ServiceTask: Requeueing request for Task 0x%p <%s> (State:%08x)", task, task->tc_Node.ln_Name, task->tc_State);
                    InternalPutMsg(PrivExecBase(SysBase)->ServicePort,
                        (struct Message *)task, SysBase);
                    break;
                }
            case TS_INVALID:
                DREMTASK("ServiceTask: Removal request for Task 0x%p <%s> (State:%08x)", task, task->tc_Node.ln_Name, task->tc_State);

                // TODO: Make sure the task has terminated..
                task->tc_State = TS_INVALID;

                /*
                 * Note tc_MemEntry list is part of the task structure which
                 * usually is also placed in tc_MemEntry. MungWall_Check()
                 * will fill freed memory and destroy our list while we are
                 * iterating or the freed memory including our list could be
                 * reused by some other task. We take special care of this by
                 * resetting ln_Succ of the last node to NULL. This way we
                 * avoid getting back to our List structure.
                 */
                task->tc_MemEntry.lh_TailPred->ln_Succ = NULL;

                for (mb = (struct MemList *)task->tc_MemEntry.lh_Head; mb; mb = mbnext)
                {
                    /* Free one MemList node */
                    mbnext = (struct MemList *)mb->ml_Node.ln_Succ;

                    DREMTASK("ServiceTask: Freeing MemList 0x%p", mb);
                    FreeEntry(mb);
                }
                break;

            default:
                if ((task->tc_Node.ln_Type == NT_TASK) || (task->tc_Node.ln_Type == NT_PROCESS))
                {
                    /* FIXME: Add fault handling here. Perhaps kernel-level GURU. */
                    DREMTASK("ServiceTask: Task 0x%p <%s> not in servicable state!\n", task, task->tc_Node.ln_Name);
                    DREMTASK("ServiceTask: State = %08X\n", task->tc_State);

                    /*
                     * Mark the task as ready to run again. Move it back to TaskReady list.
                     */
#if !defined(EXEC_REMTASK_NEEDSSWITCH)
                    task->tc_State = TS_READY;
                    Enqueue(&SysBase->TaskReady, &task->tc_Node);
#else
                    krnSysCallReschedTask(task, TS_READY);
#endif
                }
                else
                {
                    DREMTASK("ServiceTask: Called with something that is not a task??\n");
                }
                break;
            }
        }

        WaitPort(PrivExecBase(SysBase)->ServicePort);
    } while(1);
}
