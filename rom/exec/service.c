/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: exec.library's internal service task. Performs memory management according
          to task scheduler's requests.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "exec_intern.h"
#include "exec_util.h"
#include "exec_debug.h"

void ServiceTask(struct ExecBase *SysBase)
{
    DINIT("Service task started up");

    do
    { /* forever */
        struct Task *task;
        struct MemList *mb, *mbnext;

        while ((task = (struct Task *)GetMsg(PrivExecBase(SysBase)->ServicePort)))
        {
            D(bug("[exec] Service request for task 0x%p, state %d\n", task, task->tc_State));

            /*
             * If we ever need to use TSS here, we'll need to explicitly check its size here.
             * However we don't need this, because we never call so high-level libraries.
             * So, currently we ignore this.
             */

	    switch (task->tc_State)
	    {
            case TS_REMOVED:
                DREMTASK("Removal request for task 0x%p <%s>", task, task->tc_Node.ln_Name);

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

                    DREMTASK("Freeing MemList 0x%p", mb);
                    FreeEntry(mb);
                }
                break;

            default:
                /* FIXME: Add fault handling here. Perhaps kernel-level GURU. */

                /* The task is ready to run again. Move it back to TaskReady list. */
                task->tc_State = TS_READY;
                Enqueue(&SysBase->TaskReady,&task->tc_Node);

                break;
            }
        }

        WaitPort(PrivExecBase(SysBase)->ServicePort);
    } while(1);
}
