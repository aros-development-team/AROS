/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert in supervisor mode.
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <proto/kernel.h>

#include "exec_intern.h"
#include "exec_util.h"

/*
 * Display an alert via kernel.resource. This is called in a critical, hardly recoverable condition.
 * Interrupts and multitasking are disabled here.
 *
 * Note that we use shared buffer in SysBase for alert text.
 */
void Exec_SystemAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase)
{
    char *buf;

    D(bug("[SystemAlert] Code 0x%08X, type %d, data 0x%p\n", alertNum, type, data));

    /* Get the title */
    buf = Alert_AddString(PrivExecBase(SysBase)->AlertBuffer, Alert_GetTitle(alertNum));
    *buf++ = '\n';

    D(bug("[SystemAlert] Got title: %s\n", PrivExecBase(SysBase)->AlertBuffer));

    /* Get the alert text */
    buf = FormatAlert(buf, alertNum, SysBase->ThisTask, location, type, SysBase);
    FormatAlertExtra(buf, stack, type, data, SysBase);

    /* Display an alert via kernel.resource */
    KrnDisplayAlert(alertNum, PrivExecBase(SysBase)->AlertBuffer);
}
