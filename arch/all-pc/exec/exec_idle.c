/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <proto/exec.h>

#include "exec_intern.h"

#include "etask.h"

void IdleTask(struct ExecBase *SysBase)
{
    D(
        struct Task *thisTask = FindTask(NULL);
        int cpunum = GetCPUNumber();
    
        bug("[IDLE:%03d] %s started up\n", cpunum, thisTask->tc_Node.ln_Name);
    )

    do
    {
        /* forever */
        D(bug("[IDLE:%03d] CPU has idled for %d seconds..\n", cpunum, GetIntETask(thisTask)->iet_CpuTime.tv_secs));
    } while(1);
}
