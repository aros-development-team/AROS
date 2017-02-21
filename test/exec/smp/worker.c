/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include <exec/tasks.h>
#include <exec/ports.h>

#include "work.h"

/*
 * This Task processes work received
 */
void SMPTestWorker(struct ExecBase *SysBase)
{
    struct Task *thisTask = FindTask(NULL);
    D(
        int cpunum = KrnGetCPUNumber();

        bug("[SMP-Test:Worker.%03d] %s task started\n", cpunum, thisTask->tc_Node.ln_Name);
    )
    struct SMPWorker *worker = thisTask->tc_UserData;
    struct SMPWorkMessage *workMsg;
    BOOL doWork = TRUE;

    D(bug("[SMP-Test:Worker.%03d] worker data @ 0x%p\n", cpunum, worker);)
    
    if ((worker) && (worker->smpw_MasterPort))
    {
        D(bug("[SMP-Test:Worker.%03d] work Master MsgPort @ 0x%p\n", cpunum, worker->smpw_MasterPort);)
        worker->smpw_MsgPort = CreateMsgPort();
        D(bug("[SMP-Test:Worker.%03d] work MsgPort @ 0x%p\n", cpunum, worker->smpw_MsgPort);)

        while (doWork)
        {
            IPTR workType;

            /* we are ready for work .. */
            worker->smpw_Node.ln_Type = 1;
            WaitPort(worker->smpw_MsgPort);

            while((workMsg = (struct SMPWorkMessage *) GetMsg(worker->smpw_MsgPort)))
            {
                D(bug("[SMP-Test:Worker.%03d] work received (Msg @ 0x%p)\n", cpunum, workMsg);)

                /* cache the requested work and free the message ... */
                workType = workMsg->smpwm_Type;
                FreeMem(workMsg, sizeof(struct SMPWorkMessage));
                
                /* now process it .. */
                if (workType == SPMWORKTYPE_FINISHED)
                {
                    D(bug("[SMP-Test:Worker.%03d] Finished! exiting ...\n", cpunum);)

                    doWork = FALSE;
                    break;
                }
                else if (workType == SPMWORKTYPE_PROCESS)
                {
                    /*
                     * Lets do some work!
                     */
                    D(bug("[SMP-Test:Worker.%03d] Processing requested work!\n", cpunum);)

                    /* let our "parent" know we are done .. */
                    Signal(worker->smpw_MasterPort->mp_SigTask, SIGBREAKF_CTRL_D);
                }
            }
        }
        DeleteMsgPort(worker->smpw_MsgPort);
        Remove(&worker->smpw_Node);
        Signal(worker->smpw_MasterPort->mp_SigTask, SIGBREAKF_CTRL_C);
    }
}
