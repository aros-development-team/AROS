/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert in supervisor mode, Windows hosted version.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>

#include "exec_intern.h"
#include "exec_util.h"

void Exec_SystemAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase)
{
    UBYTE *buf;

    buf = Alert_AddString(PrivExecBase(SysBase)->AlertBuffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';
    buf = FormatAlert(buf, alertNum, SysBase->ThisTask, location, type, SysBase);
    /*
     * We have no chance to ask the user about anything,
     * so just fire the complete data.
     */
    FormatAlertExtra(buf, stack, type, data, SysBase);

    D(bug("[Alert] Message:\n%s\n", PrivExecBase(SysBase)->AlertBuffer));
    PD(SysBase).MessageBox(NULL, PrivExecBase(SysBase)->AlertBuffer,
			   "AROS guru meditation", 0x10010); /* MB_ICONERROR|MB_SETFOREGROUND */
}
