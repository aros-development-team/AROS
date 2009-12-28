#include <exec/alerts.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "etask.h"
#include "exec_util.h"

static LONG AskSuspend(struct Task *task, ULONG alertNum)
{
    LONG choice = -1;
    struct IntuitionBase *IntuitionBase = OpenLibrary("intuition.library", 36);
    
    if (IntuitionBase && IntuitionBase->FirstScreen) {
        struct EasyStruct es = {
            sizeof (struct EasyStruct),
            0,
            NULL,
	    "Program failed\n"
            "Task: #%P (%s)\n"
	    "Error: #%08lx (%s)\n"
            "Wait for disk activity to finish.",
            NULL,
        };
        UBYTE buffer[256];
        STRPTR taskName = Alert_GetTaskName(task);
        CONST_APTR args[] = {task, taskName, (CONST_APTR)alertNum, buffer};

        es.es_Title = Alert_GetTitle(alertNum);
        Alert_GetString(alertNum, buffer);
        if (alertNum & AT_DeadEnd)
            es.es_GadgetFormat = "Suspend|Reboot";
        else
            es.es_GadgetFormat = "Ok";
        choice = EasyRequestArgs(NULL, &es, NULL, args);
	CloseLibrary(IntuitionBase);
    }
    return choice;
}

/* This function posts alerts in user-mode via Intuition requester.
   Returns initial alert code if something fails and 0 if it was a recoverable
   alert and everything went ok.
   Note that in case of some crashes (e.g. corrupt memory list) this function
   may crash itself, and this has to be handled on a lower level. This is
   why we do this trick with iet_LastAlert */

ULONG Exec_UserAlert(ULONG alertNum, struct Task *task)
{
    struct IntETask *iet;
    STRPTR taskName;
    LONG res;

    /* Protect ourselves agains really hard crashes where SysBase->ThisTask is NULL.
       Obviously we won't go far away in such a case */
    if (!task)
        return alertNum;    
    /* Get internal task structure */
    iet = GetIntETask(task);
    /* If we already have alert number for this task, we are in double-crash during displaying
       intuition requester. Well, take the initial alert code (because it's more helpful to the programmer)
       and proceed with arch-specific Alert() */
    if (iet->iet_LastAlert[1])
	return iet->iet_LastAlert[1];
    /* Otherwise we can try to put up Intuition requester first. Store alert code in order in ETask
       in order to indicate crash condition */
    iet->iet_LastAlert[1] = alertNum;
    /* Issue a requester */
    res = AskSuspend(task, alertNum);
    /* If we managed to get here, everything went OK, remove crash indicator */
    iet->iet_LastAlert[1] = 0;
    /* If AskSuspend() failed, fail back to arch-specific Alert()*/
    if (res == -1)
	return alertNum;
    /* Halt if we need to */
    if (alertNum & AT_DeadEnd)
    {
        if (res == 0) {
/*          ShowImminentReset();*/
	    ColdReboot();
	    /* In case if ColdReboot() doesn't work */
            ShutdownA(SD_ACTION_COLDREBOOT);
	}
        /* Well, stop if the user wants so (or if the reboot didn't work at all) */
        Wait(0);
    }
    /* Otherwise return happily */
    return 0;
}
