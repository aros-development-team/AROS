#include <exec/alerts.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "exec_util.h"

static LONG AskSuspend(const TEXT *taskName, ULONG alertNum)
{
    struct IntuitionBase *IntuitionBase;
    struct EasyStruct es =
    {
        sizeof (struct EasyStruct),
        0,
        NULL,
        "%s\nProgram failed (error #%08lx).\n"
            "Wait for disk activity to finish.",
        NULL,
    };
    CONST_APTR args[] = {taskName, (CONST_APTR)alertNum};
    LONG choice = -1;

    es.es_Title = Alert_GetTitle(alertNum);
    if (alertNum & AT_DeadEnd)
        es.es_GadgetFormat = "Suspend|Reboot";
    else
        es.es_GadgetFormat = "Ok";
    IntuitionBase = OpenLibrary("intuition.library", 0);
    if (IntuitionBase != NULL)
    {
        if (IntuitionBase->FirstScreen != NULL)
        {
            choice = EasyRequestArgs(NULL, &es, NULL, args);
        }
	CloseLibrary(IntuitionBase);
    }

    return choice;
}

/* This function posts alerts in user-mode via Intuition requester.
   Returns 0 if something fails (for example Intuition is not initialised yet)
   and 1 if it was a recoverable alert and everything went ok.
   Note that in case of some crashes (e.g. corrupt memory list) this function
   may crash itself, and this has to be handler on a lower level. */

ULONG Exec_UserAlert(ULONG alertNum)
{
    struct Task *task = FindTask(NULL);
    STRPTR taskName = Alert_GetTaskName(task);
    LONG res = AskSuspend(taskName, alertNum);
    
    if (res == -1)
        return 0;
    if (alertNum & AT_DeadEnd)
    {
        if (res == 0) {
/*          ShowImminentReset();*/
	    ColdReboot();
	    /* In case if ColdReboot() doesn't work */
            ShutdownA(SD_ACTION_COLDREBOOT);
	}
        /* Well, stop if the used wants so (or if the reboot didn't work) */
        Wait(0);
    }
    return 1;
}
