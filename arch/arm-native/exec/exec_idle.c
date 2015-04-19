/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include "exec_intern.h"
#include "kernel_intern.h"
#include "kernel_cpu.h"
#include "kernel_syscall.h"

void IdleTask(struct ExecBase *SysBase)
{
    D(bug("[Kernel] Idle task started up\n"));

    asm volatile ("swi %[swi_no]" : : [swi_no] "I" (SC_SUPERSTATE) : "lr");
    asm volatile ("swi %[swi_no]" : : [swi_no] "I" (SC_STI) : "lr");

    do
    { /* forever */
        D(bug("[IDLE] Nothing to do ..\n"));
        asm volatile("mov r0, #0\n\t mcr p15, 0, r0, c7, c0, 4":::"r0");
    } while(1);
}
