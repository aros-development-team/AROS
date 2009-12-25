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
#include <string.h>

#include "../kernel/hostinterface.h"
#include "alertstrings.h"

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

    buf = Alert_AddString(buffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';
    buf = NewRawDoFmt(fmtstring, RAWFMTFUNC_STRING, buf, SysBase->ThisTask, Alert_GetTaskName(SysBase->ThisTask));
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
