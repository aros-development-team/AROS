/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert, Android-hosted version
    Lang: english
*/

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"


void Exec_SystemAlert(ULONG alertNum, struct ExecBase *SysBase)
{
    UBYTE *buf;

    buf = Alert_AddString(PrivExecBase(SysBase)->AlertBuffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';
    FormatAlert(buf, alertNum, SysBase->ThisTask, SysBase);

    /*
     * Display an alert via Java interface. This takes a long time and we don't want
     * task switcher to mess with us, so Disable() before.
     */
    PD(SysBase).DisplayAlert(PrivExecBase(SysBase)->AlertBuffer);
    AROS_HOST_BARRIER
}
