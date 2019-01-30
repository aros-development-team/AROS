/*
    Copyright � 1995-2015, The AROS Development Team. All rights reserved.
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
    int cpunum;
    register int tmp asm("r0");
    asm volatile ("swi %[swi_no]" : "=r"(tmp) : [swi_no] "I" (SC_GETCPUNUMBER) : "lr");
    cpunum = tmp;
    (void)cpunum;
    (void)thisTask;
#endif

    D(bug("[IDLE:%02d] %s started up\n", cpunum, thisTask->tc_Node.ln_Name));

    asm volatile ("swi %[swi_no]" : : [swi_no] "I" (SC_SUPERSTATE) : "lr");
    asm volatile ("swi %[swi_no]" : : [swi_no] "I" (SC_STI) : "lr");

    do
    {
        /* forever */
        D(bug("[IDLE:%02d] CPU has idled for %d seconds..\n", cpunum, GetIntETask(thisTask)->iet_CpuTime.tv_sec));
        asm volatile("mov r0, #0\n\t mcr p15, 0, r0, c7, c0, 4":::"r0");
    } while(1);
}
