/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/tasks.h>

#include "exec_intern.h"
#include "etask.h"

#define BILLION 1000000000L

UQUAD getIETPriv1(struct Task *task)
{
    return IntETask(task->tc_UnionETask.tc_ETask)->iet_private1;
}

void setIETPriv1(struct Task *task, UQUAD value)
{
    IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 = value;
}

void setIETPrivTime(struct Task *task, ULONG cyles, ULONG sec, ULONG nsec)
{
    /* Increase CPU Usage cycles */
    IntETask(task->tc_UnionETask.tc_ETask)->iet_private2 += cyles;

    IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec += nsec;
    IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_sec  += sec;
    while(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec >= BILLION)
    {
        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec -= BILLION;
        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_sec++;
    }
}
