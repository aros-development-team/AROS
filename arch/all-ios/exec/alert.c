/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id: alert.c 34578 2010-10-04 07:19:30Z sonic $

    Desc: Display an alert, iOS-hosted version
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

AROS_LH1(void, Alert,
	 AROS_LHA(ULONG, alertNum, D7),
	 struct ExecBase *, SysBase, 18, Exec)
{
    AROS_LIBFUNC_INIT

    struct Task *task = SysBase->ThisTask;
    UBYTE *buf;

    /* If we are running in user mode we should first try to report a problem using AROS'
       own way to do it */
    if (!KrnIsSuper())
    {
        alertNum = Exec_UserAlert(alertNum, task, SysBase);
	if (!alertNum)
	    return;
    }

    /* User-mode alert routine failed */
    Disable();

    buf = Alert_AddString(PrivExecBase(SysBase)->AlertBuffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';
    FormatAlert(buf, alertNum, task, SysBase);

    /*
     * Display an alert using UIKit. This takes a long time and we don't want
     * task switcher to mess with us, so Disable() before.
     */
    PD(SysBase).DisplayAlert(PrivExecBase(SysBase)->AlertBuffer);
    AROS_HOST_BARRIER

    if (alertNum & AT_DeadEnd)
    {
	/* Um, we have to do something here in order to prevent the
	   computer from continuing... */
	PD(SysBase).Reboot(TRUE);
	AROS_HOST_BARRIER
	PD(SysBase).SysIFace->exit(0);
	AROS_HOST_BARRIER
    }
    Enable();

    AROS_LIBFUNC_EXIT
}
