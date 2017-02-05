/*
    Copyright © 2015-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <proto/exec.h>
#include <proto/kernel.h>

#include "exec_intern.h"

#include "etask.h"

void IdleTask(struct ExecBase *SysBase)
{
    D(
        struct Task *thisTask = FindTask(NULL);
        int cpunum = KrnGetCPUNumber();
        ULONG lastcount = 0, current;

        bug("[IDLE:%03d] %s started up\n", cpunum, thisTask->tc_Node.ln_Name);
    )

    do
    {
        /* forever */
        D(
            current = GetIntETask(thisTask)->iet_CpuTime.tv_secs;
            if (current != lastcount)
            {
                lastcount = current;
                bug("[IDLE:%03d] CPU has idled for %d seconds..\n", cpunum, lastcount);
            }
        )
    } while(1);
}
