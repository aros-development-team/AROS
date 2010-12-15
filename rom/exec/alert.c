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
        This is actually a poor-man implementation which prints alert information
	to the debug output. Only this thing works everywhere and only this thing
	can be called from within interrupts and traps. It's done so just because
	it's better than nothing. Well, some day things will change...

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[exec] Alert 0x%08X\n", alertNum));
    
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
    AROS_LIBFUNC_EXIT
}
