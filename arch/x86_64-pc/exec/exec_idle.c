/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <proto/exec.h>
#include "exec_intern.h"
#include "kernel_intern.h"
#include "kernel_cpu.h"
#include "kernel_syscall.h"

#include "etask.h"

void IdleTask(struct ExecBase *SysBase)
{
#if defined(DEBUG)
    struct Task *thisTask = FindTask(NULL);
    int cpunum = GetCPUNumber();
#endif

    D(bug("[IDLE:%02d] %s started up\n", cpunum, thisTask->tc_Node.ln_Name));

    do
    {
        /* forever */
        D(bug("[IDLE:%02d] CPU has idled for %d seconds..\n", cpunum, GetIntETask(thisTask)->iet_CpuTime.tv_secs));
    } while(1);
}
