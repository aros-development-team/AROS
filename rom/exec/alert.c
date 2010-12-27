/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

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

#include "exec_intern.h"
#include "exec_util.h"
#include "etask.h"

/* x86/64 kernel.resource doesn't have KrnIsSuper() */
#ifndef KrnIsSuper
#define KrnIsSuper() 0
#endif

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *task = SysBase->ThisTask;
    struct IntETask *iet = NULL;

    D(bug("[exec] Alert 0x%08X\n", alertNum));

    if (task)
    {
	iet = GetIntETask(task);

	/* Do we already have location set? */
	if (!iet->iet_AlertLocation)
	{
	    /* If no, the location is where we were called from */
	    APTR fp = AROS_GET_FP;

	    D(bug("[Alert] Frame pointer 0x%p\n", fp));

	    iet->iet_AlertStack = UnwindFrame(fp, &iet->iet_AlertLocation);
	    D(bug("[Alert] Previous frame 0x%p, caller 0x%p\n", iet->iet_AlertStack, iet->iet_AlertLocation));
	}
    }

    /*
     * If we are running in user mode we should first try to report a problem
     * using Intuition display.
     */
    if (!KrnIsSuper())
    {
        alertNum = Exec_UserAlert(alertNum, SysBase);
	if (!alertNum)
	    return;
    }

    /*
     * We're here if Intuition failed. Use safe (but not so
     * system and user-friendly) way to post alerts.
     */
    Disable();
    Exec_SystemAlert(alertNum, SysBase);
    Enable();

    if (alertNum & AT_DeadEnd)
    {
	/* Um, we have to do something here in order to prevent the
	   computer from continuing... */
	ColdReboot();
	ShutdownA(SD_ACTION_COLDREBOOT);
    }

    /*
     * We displayed an alert in supervisor mode, but this still was a recoverable alert.
     * Clear alert status by clearing AlertCode and AlertLocation.
     */
    if (iet)
    {
	iet->iet_AlertCode = 0;
	iet->iet_AlertLocation = NULL;
    }

    AROS_LIBFUNC_EXIT
}
