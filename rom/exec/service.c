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
    struct Task *task;
    struct MemList *mb, *mbnext;

    DINIT("ServiceTask: Started up");

    do
    { /* forever */


        while ((task = (struct Task *)GetMsg(PrivExecBase(SysBase)->ServicePort)))
        {
            DREMTASK("ServiceTask: Request for task 0x%p, state %08X\n", task, task->tc_State);

            /*
             * If we ever need to use TSS here, we'll need to explicitly check its size here.
             * However we don't need this, because we never call so high-level libraries.
             * So, currently we ignore this.
             */

	    switch (task->tc_State)
	    {
            case TS_REMOVED:
                DREMTASK("ServiceTask: Requeueing request for task 0x%p <%s>", task, task->tc_Node.ln_Name);
                InternalPutMsg(PrivExecBase(SysBase)->ServicePort,
                    (struct Message *)task, SysBase);
                break;
            case TS_INVALID:
                DREMTASK("ServiceTask: Removal request for task 0x%p <%s>", task, task->tc_Node.ln_Name);

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
                /* FIXME: Add fault handling here. Perhaps kernel-level GURU. */
                DREMTASK("ServiceTask: task 0x%p <%s> not in tombstoned state!\n", task, task->tc_Node.ln_Name);
                DREMTASK("ServiceTask: state = %08X\n", task->tc_State);
                /* The task is ready to run again. Move it back to TaskReady list. */
#if !defined(EXEC_REMTASK_NEEDSSWITCH)
                task->tc_State = TS_READY;
                Enqueue(&SysBase->TaskReady, &task->tc_Node);
#else
                krnSysCallReschedTask(task, TS_READY);
#endif
                break;
            }
        }

        WaitPort(PrivExecBase(SysBase)->ServicePort);
    } while(1);
}
