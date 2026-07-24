/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    AArch64 idle task.
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
    register int tmp asm("x0");
    asm volatile ("svc %[svc_no]" : "=r"(tmp) : [svc_no] "I" (SC_GETCPUNUMBER) : "x30");
    cpunum = tmp;
    (void)cpunum;
    (void)thisTask;
#endif

    D(bug("[IDLE:%02d] %s started up\n", cpunum, thisTask->tc_Node.ln_Name));

    /*
     * Stay at the task level (EL1t) - do NOT SuperState to EL1h. An interrupt
     * taken at EL1h vectors to __el1_irq, which does not reschedule, so a task
     * woken by an interrupt (e.g. a timer.device ReplyMsg) while we idle would
     * never be dispatched. Idling at EL1t routes the wakeup IRQ through
     * __el0_irq -> core_ExitInterrupt -> reschedule. Just enable interrupts.
     */
    asm volatile ("svc %[svc_no]" : : [svc_no] "I" (SC_STI) : "x30");

    do
    {
        /* forever */
        D(bug("[IDLE:%02d] CPU has idled for %d seconds..\n", cpunum, GetIntETask(thisTask)->iet_CpuTime.tv_sec));
        asm volatile("wfi");
    } while(1);
}
