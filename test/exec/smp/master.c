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
 * This Task sends work out to the "Workers" to process
 */
void SMPTestMaster(struct ExecBase *SysBase)
{
    struct Task *thisTask = FindTask(NULL);
    D(
        int cpunum = KrnGetCPUNumber();

        bug("[SMP-Test:Master.%03d] %s task started\n", cpunum, thisTask->tc_Node.ln_Name);
    )
    struct SMPMaster *workMaster = thisTask->tc_UserData;
    struct SMPWorkMessage *workMsg;
    struct SMPWorker *coreWorker;

    if (workMaster)
    {
        IPTR msgNo;

        D(bug("[SMP-Test:Master.%03d] worker list @ 0x%p\n", cpunum, &workMaster->smpm_Workers);)

        for (msgNo = 0; msgNo < 1000; msgNo++)
        {
            if ((workMsg = (struct SMPWorkMessage *)AllocMem(sizeof(struct SMPWorkMessage), MEMF_CLEAR)) != NULL)
            {
                /* prepare the work to be done ... */
                workMsg->smpwm_Type = SPMWORKTYPE_PROCESS;

                /* send out the work to an available worker... */
                do {
                    ForeachNode(&workMaster->smpm_Workers, coreWorker)
                    {
                        if ((workMsg) && (coreWorker->smpw_Node.ln_Type == 1) && (coreWorker->smpw_MsgPort))
                        {
                            D(bug("[SMP-Test] %s: Sending work @ 0x%p to worker @ 0x%p\n", __func__, workMsg, coreWorker);)
                            coreWorker->smpw_Node.ln_Type = 0;
                            PutMsg(coreWorker->smpw_MsgPort, (struct Message *)workMsg);
                            workMsg = NULL;
                        }
                    }
                } while (workMsg);
            }
        }
    }

    workMaster->smpm_Master = NULL;
    Signal(workMaster->smpm_MasterPort->mp_SigTask, SIGBREAKF_CTRL_D);

    D(bug("[SMP-Test:Master.%03d] work complete\n", cpunum);)
}
