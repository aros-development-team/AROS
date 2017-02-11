/*
    Copyright © 2015-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <proto/exec.h>

#define __AROS_KERNEL__

#include "exec_intern.h"

#include "etask.h"

void IdleTask(struct ExecBase *SysBase)
{
    D(
        struct Task *thisTask = FindTask(NULL);
        struct IntETask *taskIntEtask;
        int cpunum = KrnGetCPUNumber();
        ULONG lastcount = 0, current;

        bug("[IDLE:%03d] %s task started\n", cpunum, thisTask->tc_Node.ln_Name);
    )

    do
    {
        /* forever */
        D(
            if ((taskIntEtask = GetIntETask(thisTask)) != NULL)
            {
                current = taskIntEtask->iet_CpuTime.tv_secs;
                if (current != lastcount)
                {
                    lastcount = current;
                    bug("[IDLE:%03d] CPU has idled for %d seconds..\n", cpunum, lastcount);
                }
            }
            else
            {
                bug("[IDLE:%03d] Failed to get IntETask\n", cpunum);
            }
        )
    } while(1);
}
