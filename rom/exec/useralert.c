#include <aros/debug.h>
#include <exec/alerts.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/kernel.h>

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

#define ALERT_BUFFER_SIZE 2048

static char *startstring = "Program failed\n";
static char *endstring   = "\nWait for disk activity to finish.";

static LONG AskSuspend(struct Task *task, ULONG alertNum, struct ExecBase *SysBase)
{
    LONG choice = -1;
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);

    if (IntuitionBase)
    {
        char *buffer = AllocMem(ALERT_BUFFER_SIZE, MEMF_ANY);

        if (buffer)
        {
	    char *buf;
	    struct EasyStruct es = {
        	sizeof (struct EasyStruct),
            	0,
            	NULL,
	    	buffer,
            	NULL,
            };

	    buf = Alert_AddString(buffer, startstring);
	    buf = FormatAlert(buf, alertNum, task, SysBase);
	    buf = Alert_AddString(buf, endstring);
	    *buf = 0;

	    es.es_Title = Alert_GetTitle(alertNum);
	    if (alertNum & AT_DeadEnd)
        	es.es_GadgetFormat = "Suspend|Reboot";
            else
            	es.es_GadgetFormat = "Continue";

	    D(bug("[UserAlert] Body text:\n%s\n", buffer));
	    choice = EasyRequestArgs(NULL, &es, NULL, NULL);
	    
	    FreeMem(buffer, ALERT_BUFFER_SIZE);
	}

	CloseLibrary(&IntuitionBase->LibNode);
    }
    return choice;
}

/*
 * This function posts alerts in user-mode via Intuition requester.
 * Returns initial alert code if something fails and 0 if it was a recoverable
 * alert and everything went ok.
 * Note that in case of some crashes (e.g. corrupt memory list) this function
 * may crash itself, and this has to be handled on a lower level. This is
 * why we do this trick with iet_AlertCode
 */
ULONG Exec_UserAlert(ULONG alertNum, struct Task *task, struct ExecBase *SysBase)
{
    struct IntETask *iet;
    LONG res;

    /* Protect ourselves agains really hard crashes where SysBase->ThisTask is NULL.
       Obviously we won't go far away in such a case */
    if (!task)
        return alertNum;

    /* Get internal task structure */
    iet = GetIntETask(task);
    /*
     * If we already have alert number for this task, we are in double-crash during displaying
     * intuition requester. Well, take the initial alert code (because it's more helpful to the programmer)
     * and proceed with arch-specific Alert().
     * Since this is a double-crash, we may append AT_DeadEnd flag if our situation has become unrecoverable.
     */
    if (iet->iet_AlertCode)
	return iet->iet_AlertCode | (alertNum & AT_DeadEnd);

    /* Otherwise we can try to put up Intuition requester first. Store alert code in order in ETask
       in order to indicate crash condition */
    iet->iet_AlertCode = alertNum;
    /* Issue a requester */
    res = AskSuspend(task, alertNum, SysBase);
    /* If AskSuspend() failed, fail back to safe-mode alert */
    if (res == -1)
	return alertNum;

    /* Halt if we need to */
    if (alertNum & AT_DeadEnd)
    {
        if (res == 0)
        {
	    ColdReboot();
	    /* In case if ColdReboot() doesn't work */
            ShutdownA(SD_ACTION_COLDREBOOT);

            D(bug("[UserAlert] Returned from ShutdownA()!\n"));
	}
        /* Well, stop if the user wants so (or if the reboot didn't work at all) */
        Wait(0);
    }

    /* Otherwise remove crash indicator and return happily */
    iet->iet_AlertCode = 0;
    return 0;
}
