#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H
/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <exec/tasks.h>

struct X86SchedulerPrivate
{
    struct Task         *RunningTask;   /* Currently running task on this core  */

    ULONG               ScheduleFlags;
    UWORD               Quantum;        /* # of ticks, a task may run                   */
    UWORD               Elapsed;        /* # of ticks, the current task has run         */
    BYTE                IDNestCnt;
    BYTE                TDNestCnt;
};
#endif

BOOL core_Schedule(void);			/* Reschedule the current task if needed */
void core_Switch(void);				/* Switch away from the current task     */
struct Task *core_Dispatch(void);		/* Select the new task for execution     */

#endif /* !KERNEL_SCHEDULER_H */