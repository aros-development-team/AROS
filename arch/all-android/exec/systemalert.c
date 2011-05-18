/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert, Android-hosted version
    Lang: english
*/

#include <aros/atomic.h>
#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"


void Exec_SystemAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase)
{
    UBYTE *buf;

    buf = Alert_AddString(PrivExecBase(SysBase)->AlertBuffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';
    buf = FormatAlert(buf, alertNum, SysBase->ThisTask, location, type, SysBase);
    FormatAlertExtra(buf, stack, type, data, SysBase);

    /*
     * Explicitly disable task switching.
     * Yes, we are in Disable(). However Dalvik VM will enable SIGALRM.
     * This means Disable()d state will be broken. Additionally it messes
     * with stack or threads, which will cause AN_StackProbe guru during
     * displaying an alert if we don't do this.
     */
    Forbid();

    /* Display the alert via Java interface. */
    PD(SysBase).DisplayAlert(PrivExecBase(SysBase)->AlertBuffer);

    /*
     * Fix up interrupts state before Permit().
     * Yes, there will be Enable() after return, but let's not
     * forget about nesting count.
     */
    KrnCli();
    Permit();
}
