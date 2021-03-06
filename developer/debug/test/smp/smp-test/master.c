/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
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

        bug("[SMP-Test:Master.%03d] %s: %s task started\n", cpunum, __func__, thisTask->tc_Node.ln_Name);
    )
    struct SMPMaster *workMaster = thisTask->tc_UserData;
    struct SMPWorkMessage *workMsg;
    struct SMPWorker *coreWorker;

    if (workMaster)
    {
        IPTR msgWork = (workMaster->smpm_Width * workMaster->smpm_Height) / (256 / workMaster->smpm_WorkerCount);
        IPTR msgNo;

        D(
            bug("[SMP-Test:Master.%03d] %s: worker list @ 0x%p (%d workers)\n", cpunum, __func__, &workMaster->smpm_Workers, workMaster->smpm_WorkerCount);
            bug("[SMP-Test:Master.%03d] %s: worker workload = %d\n", cpunum, __func__, msgWork);
        )

        for (msgNo = 0; msgNo < ((workMaster->smpm_Width * workMaster->smpm_Height)/ msgWork); msgNo++)
        {
            if ((workMsg = (struct SMPWorkMessage *)AllocMem(sizeof(struct SMPWorkMessage), MEMF_CLEAR)) != NULL)
            {
                /* prepare the work to be done ... */
                if (workMaster->smpm_Buddha)
                    workMsg->smpwm_Type = SPMWORKTYPE_BUDDHA;
                else
                    workMsg->smpwm_Type = SPMWORKTYPE_MANDLEBROT;
                workMsg->smpwm_Buffer = workMaster->smpm_WorkBuffer;
                workMsg->smpwm_Width = workMaster->smpm_Width;
                workMsg->smpwm_Height = workMaster->smpm_Height;
                workMsg->smpwm_Start = msgNo * msgWork;
                workMsg->smpwm_End = workMsg->smpwm_Start + msgWork - 1;
                if  ((((workMaster->smpm_Width * workMaster->smpm_Height)/ msgWork) <= (msgNo + 1)) &&
                      (workMsg->smpwm_End < (workMaster->smpm_Width * workMaster->smpm_Height)))
                    workMsg->smpwm_End = workMaster->smpm_Width * workMaster->smpm_Height;

                /* send out the work to an available worker... */
                do {
                    ForeachNode(&workMaster->smpm_Workers, coreWorker)
                    {
                        if ((workMsg) && (coreWorker->smpw_Node.ln_Type == 1) && (coreWorker->smpw_MsgPort))
                        {
                            D(bug("[SMP-Test:Master.%03d] %s: Sending work @ 0x%p to worker @ 0x%p\n", cpunum, __func__, workMsg, coreWorker);)
                            coreWorker->smpw_Node.ln_Type = 0;
                            PutMsg(coreWorker->smpw_MsgPort, (struct Message *)workMsg);
                            workMsg = NULL;
                            break;
                        }
                    }
                } while (workMsg);
            }
        }
    }

    workMaster->smpm_Master = NULL;
    Signal(workMaster->smpm_MasterPort->mp_SigTask, SIGBREAKF_CTRL_D);

    D(bug("[SMP-Test:Master.%03d] %s: work complete\n", cpunum, __func__);)
}
