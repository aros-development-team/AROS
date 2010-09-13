/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

#include "etask.h"
#include "exec_intern.h"
#include "exec_util.h"

static UBYTE *const fmtstring = "Task 0x%P - %s\n";
static UBYTE *const errstring = "Error %08lx - ";
static UBYTE *const locstring = "PC 0x%P\n";
static UBYTE *const modstring = "Module %s Segment %u %s (0x%P) Offset 0x%P\n";
static UBYTE *const funstring = "Function %s (0x%P) Offset 0x%P\n";

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE buffer[512], *buf;
    struct Task *task = SysBase->ThisTask;

    /* If we are running in user mode we should first try to report a problem using AROS'
       own way to do it */
/* Disabled in order to test new crash location finder - Sonic
    if (!KrnIsSuper())
    {
        alertNum = Exec_UserAlert(alertNum, task, SysBase);
	if (!alertNum)
	    return;
    }
*/
    buf = Alert_AddString(buffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';
    buf = NewRawDoFmt(fmtstring, RAWFMTFUNC_STRING, buf, task, Alert_GetTaskName(task));
    buf = NewRawDoFmt(errstring, RAWFMTFUNC_STRING, --buf, alertNum);
    buf = Alert_GetString(alertNum, --buf);

    if (task) {
	struct IntETask *iet = GetIntETask(task);

	if (iet->iet_AlertLocation) {
	    char *modname, *segname, *symname;
	    void *segaddr, *symaddr;
	    unsigned int segnum;

	    *buf++ = '\n';
	    buf = NewRawDoFmt(locstring, RAWFMTFUNC_STRING, buf, iet->iet_AlertLocation);

	    if (KrnDecodeLocation(iet->iet_AlertLocation,
				  KDL_ModuleName , &modname, KDL_SegmentNumber, &segnum ,
				  KDL_SegmentName, &segname, KDL_SegmentStart , &segaddr,
				  KDL_SymbolName , &symname, KDL_SymbolStart  , &symaddr,
				  TAG_DONE))
	    {
		if (!segname)
		    segname = "- unknown -";
		buf = NewRawDoFmt(modstring, RAWFMTFUNC_STRING, --buf, modname, segnum, segname, segaddr, iet->iet_AlertLocation - segaddr);
		if (symaddr) {
		    if (!symname)
			symname = "- unknown -";
		    buf = NewRawDoFmt(funstring, RAWFMTFUNC_STRING, --buf, symname, symaddr, iet->iet_AlertLocation - symaddr);
		}
	    }
	}
    }

    /* Display an alert using Windows message box */
    Disable();
    D(bug("[Alert] Message:\n%s\n", buffer));
    PD(SysBase).MessageBox(NULL, buffer, "AROS guru meditation", 0x10010); /* MB_ICONERROR|MB_SETFOREGROUND */

    if (alertNum & AT_DeadEnd)
    {
	/* Um, we have to do something here in order to prevent the
	   computer from continuing... */
	ColdReboot();
    }
    Enable();

    AROS_LIBFUNC_EXIT
}
