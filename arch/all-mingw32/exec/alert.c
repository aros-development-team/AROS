/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id: alert.c 32172 2009-12-25 11:40:20Z sonic $

    Desc: Display an alert.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <string.h>

#include "../kernel/hostinterface.h"
#include "etask.h"
#include "exec_util.h"

static UBYTE *const fmtstring = "Task %08lx - %s\n";
static UBYTE *const errstring = "Error %08lx - ";
extern struct HostInterface *HostIFace;

/*****************************************************************************

    NAME */

	AROS_LH1(void, Alert,

/*  SYNOPSIS */
	AROS_LHA(ULONG, alertNum, D7),

/*  LOCATION */
	struct ExecBase *, SysBase, 18, Exec)

/*  FUNCTION
	Alerts the user of a serious system problem.

    INPUTS
	alertNum - This is a number which contains information about
		the reason for the call.

    RESULT
	This routine may return, if the alert is not a dead-end one.

    NOTES
	You should not call this routine because it halts the machine,
	displays the message and then may reboot it.

    EXAMPLE
	// Dead-End alert: 680x0 Access To Odd Address
	Alert (0x80000003);

    BUGS

    SEE ALSO

    INTERNALS
        This is a very basic implementation for Windows-hosted AROS. It displays
	alerts as Windows message boxes.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE buffer[256], *buf;
    struct Task *task = SysBase->ThisTask;
    struct IntETask *iet;
    ULONG ret;
    
    /* The following part has any sense only in usermode and only if we have task. */
    if (!KrnIsSuper() && task) {
        /* Get internal task structure */
        iet = GetIntETask(task);
	/* If we already have alert number for this task, we are in double-crash during displaying
           intuition requester. Well, take the initial alert code (because it's more helpful to the programmer)
	   and proceed with system alert */
	if (iet->iet_LastAlert[1])
	    alertNum = iet->iet_LastAlert[1];
	else {
	    /* Otherwise we can try to put up Intuition requester first. Store alert code in order in ETask
	       in order to indicate crash condition */
	    iet->iet_LastAlert[1] = alertNum;
	    /* Issue a requester */
	    ret = Exec_UserAlert(alertNum);
	    /* If we managed to get here, everything went OK, remove crash indicator */
	    iet->iet_LastAlert[1] = 0;
	    /* Return if Exec_UserAlert() allows us to do it */
	    if (ret)
	        return;
	}
    }

    /* In future the code below this point should go to arch-specific sys_alert.c, and rom/exec/useralert.c
       should be merged with rom/exec/alert.c.
       Note that first part of this function differs only in KrnIsSuper() because some ports still don't
       have kernel.resource */

    buf = Alert_AddString(buffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';
    buf = NewRawDoFmt(fmtstring, RAWFMTFUNC_STRING, buf, task, Alert_GetTaskName(task));
    buf = NewRawDoFmt(errstring, RAWFMTFUNC_STRING, --buf, alertNum);
    Alert_GetString(alertNum, --buf);
    Disable();
    D(bug("[Alert] Message:\n%s\n", buffer));
    HostIFace->_Alert(buffer);
    
    if (alertNum & AT_DeadEnd)
    {
	/* Um, we have to do something here in order to prevent the
	   computer from continuing... */
	HostIFace->_Shutdown(SD_ACTION_COLDREBOOT);
    }
    Enable();

    AROS_LIBFUNC_EXIT
}
