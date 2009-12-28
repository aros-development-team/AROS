/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert.
    Lang: english
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <string.h>

#include "exec_util.h"

#define ALERT_WIDTH 80

static UBYTE *const fmtstring = "Task %08lx - %s";
static UBYTE *const errstring = "Error %08lx - ";

static void PrintChars(char c, ULONG n)
{
    while (n--)
        RawPutChar(c);
}

static void PrintCentered(char *str)
{
    ULONG s = ALERT_WIDTH - 2 - strlen(str);
    
    RawPutChar('#');
    if (s & 1)
        RawPutChar(' ');
    s >>= 1;
    PrintChars(' ', s);
    while (*str)
        RawPutChar(*str++);
    PrintChars(' ', s);
    RawPutChar('#');
    RawPutChar('\n');
}

static void PrintFrame(void)
{
    PrintChars('#', ALERT_WIDTH);
    RawPutChar('\n');
}

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

    UBYTE buffer[256], *buf;
    struct Task *task = SysBase->ThisTask;
    struct IntETask *iet;
    ULONG ret;
    
    /* The following part has any sense only in usermode and only if we have task. */
    if (task) {
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
    PrintFrame();
    PrintCentered(Alert_GetTitle(alertNum));
    NewRawDoFmt(fmtstring, RAWFMTFUNC_STRING, buffer, SysBase->ThisTask, Alert_GetTaskName(SysBase->ThisTask));
    PrintCentered(buffer);
    buf = NewRawDoFmt(errstring, RAWFMTFUNC_STRING, buffer, alertNum);
    Alert_GetString(alertNum, --buf);
    PrintCentered(buffer);
    PrintFrame();
    RawPutChar('\n');
    
    if (alertNum & AT_DeadEnd)
    {
	/* Um, we have to do something here in order to prevent the
	   computer from continuing... */
	ColdReboot();
	ShutdownA(SD_ACTION_COLDREBOOT);
    }
    AROS_LIBFUNC_EXIT
}
