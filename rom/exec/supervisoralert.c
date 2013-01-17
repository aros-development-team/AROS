/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert passed from supervisor mode.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/rawfmt.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/kernel.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

LONG Alert_AskSuspend(struct Task *task, ULONG alertNum, char * buffer, struct ExecBase *SysBase);
void Alert_DisplayKrnAlert(struct Task * task, ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data,
        struct ExecBase *SysBase);

/*
 * This task tries to display alerts that occured in supervisor mode. The task that caused the
 * problem is available in SAT.sat_Params[1]. The code that raised Alert is responsible for
 * stopping/sanitizing the task itself. Since the task is already stopped, AT_DeadEnd is added to
 * when invoking alert functions so that user does not have an option to continue the task.
 */
void SupervisorAlertTask(struct ExecBase *SysBase)
{
    struct IntExecBase * IntSysBase = PrivExecBase(SysBase);
    char * buffer = AllocMem(ALERT_BUFFER_SIZE, MEMF_ANY);
    LONG res, i;
    struct Task * t = NULL;
    ULONG alertNum = 0;

    IntSysBase->SAT.sat_Task = FindTask(NULL);
    IntSysBase->SAT.sat_IsAvailable = TRUE;

    while(TRUE)
    {
        Wait(SIGF_SINGLE);

        Disable();
        /* Mark task as in use so that nested crash will use critical error path, see Exec_SystemAlert */
        IntSysBase->SAT.sat_IsAvailable = FALSE;
        t = (struct Task*)IntSysBase->SAT.sat_Params[1];
        alertNum = IntSysBase->SAT.sat_Params[0];
        Enable();

        res = Alert_AskSuspend(t, alertNum | AT_DeadEnd, buffer, SysBase);

        if (res == -1)
        {
            /* It was not possible to report error to user, fallback to critical error path */
            struct IntETask * iet = GetIntETask(t);
            Disable();

            Alert_DisplayKrnAlert(t, alertNum | AT_DeadEnd, iet->iet_AlertLocation, iet->iet_AlertStack,
                    iet->iet_AlertType, (APTR)&iet->iet_AlertData, SysBase);

            if (alertNum & AT_DeadEnd)
            {
                /* Um, we have to do something here in order to prevent the
                   computer from continuing... */
                ColdReboot();
                ShutdownA(SD_ACTION_COLDREBOOT);
            }

            Enable();
        }

        switch (res)
        {
        case 0:
            ShutdownA(SD_ACTION_POWEROFF);
            break;

        case 2:
            /* Suspend actually should have already happened by code raising Alert */
            break;

        case 3:
            ColdReboot();
            /* In case if ColdReboot() doesn't work */
            ShutdownA(SD_ACTION_COLDREBOOT);
            break;
        }


        Disable();
        /* Mark task as available */
        IntSysBase->SAT.sat_IsAvailable = TRUE;
        for (i = 0; i < 2; i++)
            IntSysBase->SAT.sat_Params[i] = (IPTR)NULL;
        t = NULL;
        alertNum = 0;
        Enable();
    }
}
