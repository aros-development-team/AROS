/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

    Exec_ExtAlert(alertNum, __builtin_return_address(0), CALLER_FRAME, 0, NULL, SysBase);

    AROS_LIBFUNC_EXIT
}

static const ULONG contextSizes[] =
{
    0,
    sizeof(struct ExceptionContext),
    sizeof(struct MungwallContext),
};

void Exec_ExtAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase)
{
    struct Task *task = SysBase->ThisTask;
    struct IntETask *iet = NULL;
    int supervisor = KrnIsSuper();

    D(bug("[exec] Alert 0x%08X\n", alertNum));

    if (task && (task->tc_State != TS_REMOVED))
    {
	iet = GetIntETask(task);

	/* Do we already have location set? */
	if (iet->iet_AlertFlags & AF_Location)
	{
	    /* If yes, pick it up */
	    location = iet->iet_AlertLocation;
	    stack    = iet->iet_AlertStack;
	}
	else
	{
	    if (supervisor && ((alertNum & ~AT_DeadEnd) == AN_StackProbe))
	    {
	    	/*
	    	 * Special case: AN_StackProbe issued by kernel's task dispatcher.
	    	 * Pick up data from task's context.
	    	 */
		struct ExceptionContext *ctx = iet->iet_Context;

		location = (APTR)ctx->PC;
	    	stack    = (APTR)ctx->FP;
	    	type     = AT_CPU;
	    	data     = ctx;
	    }

	    /* Set location */
	    iet->iet_AlertFlags   |= AF_Location;	    
	    iet->iet_AlertLocation = location;
	    iet->iet_AlertStack    = stack;

	    D(bug("[Alert] Previous frame 0x%p, caller 0x%p\n", iet->iet_AlertStack, iet->iet_AlertLocation));
	}

	/*
	 * Set the data if specified.
	 * This can happen only on the first call, since we can recurse only into Alert(),
	 * not into ExtAlert() directly.
	 */
	if (data)
	{
	    iet->iet_AlertType = type;
	    CopyMem(data, &iet->iet_AlertData, contextSizes[type]);
	}
	else
	    type = iet->iet_AlertType;

	/*
	 * At this point iet_AlertData is guaranteed to be set
	 * either by caller or by us.
	 */
	data = type ? &iet->iet_AlertData : NULL;	
    }
    else
    	/*
    	 * If we have no task, or the task is being removed,
    	 * we can't use the user-mode routine.
    	 */
    	supervisor = TRUE;

    /*
     * If we are running in user mode we should first try to report a problem
     * using Intuition display.
     */
    if (!supervisor)
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
    Exec_SystemAlert(alertNum, location, stack, type, data, SysBase);
    Enable();

    /*
     * We succesfully displayed an alert in supervisor mode.
     * Clear alert status by clearing respective fields in ETask.
     */
    if (iet)
    {
	ResetETask(iet);
    }

    if (alertNum & AT_DeadEnd)
    {
	/* Um, we have to do something here in order to prevent the
	   computer from continuing... */
	ColdReboot();
	ShutdownA(SD_ACTION_COLDREBOOT);
    }
}
