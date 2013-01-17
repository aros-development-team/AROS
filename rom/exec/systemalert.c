/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert in supervisor mode.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <proto/kernel.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

void Alert_DisplayKrnAlert(struct Task * task, ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data,
        struct ExecBase *SysBase)
{
    char *buf;

    /* Get the title */
    buf = Alert_AddString(PrivExecBase(SysBase)->AlertBuffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';

    D(bug("[SystemAlert] Got title: %s\n", PrivExecBase(SysBase)->AlertBuffer));

    /* Get the alert text */
    buf = FormatAlert(buf, alertNum, task, location, type, SysBase);
    FormatAlertExtra(buf, stack, type, data, SysBase);

    /* Task is not available, display an alert via kernel.resource */
    KrnDisplayAlert(alertNum, PrivExecBase(SysBase)->AlertBuffer);
}

/*
 * Display an alert via kernel.resource. This is called in a critical, hardly recoverable condition.
 * Interrupts and multitasking are disabled here.
 *
 * Note that we use shared buffer in SysBase for alert text.
 */
void Exec_SystemAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase)
{
    D(bug("[SystemAlert] Code 0x%08X, type %d, data 0x%p\n", alertNum, type, data));

    if ((SysBase->ThisTask == PrivExecBase(SysBase)->SAT.sat_Task) &&
            (PrivExecBase(SysBase)->SAT.sat_Params[1] != (IPTR) NULL))
    {
        /* SupervisorAlertTask crashed when trying to show crash information for another task */

        struct Task * t = (struct Task*)PrivExecBase(SysBase)->SAT.sat_Params[1];
        ULONG alertNum = PrivExecBase(SysBase)->SAT.sat_Params[0];
        struct IntETask * iet = GetIntETask(t);

        Alert_DisplayKrnAlert(t, alertNum | AT_DeadEnd, iet->iet_AlertLocation, iet->iet_AlertStack,
                            iet->iet_AlertType, (APTR)&iet->iet_AlertData, SysBase);
    }
    else if (PrivExecBase(SysBase)->SAT.sat_IsAvailable && !(alertNum & AT_DeadEnd))
    {
        /* SupervisorAlertTask is available, use it */

        PrivExecBase(SysBase)->SAT.sat_Params[0] = alertNum;
        PrivExecBase(SysBase)->SAT.sat_Params[1] = (IPTR)SysBase->ThisTask;

        Signal(PrivExecBase(SysBase)->SAT.sat_Task, SIGF_SINGLE);
    }
    else
    {
        Alert_DisplayKrnAlert(SysBase->ThisTask, alertNum, location, stack, type, data, SysBase);
    }
}
