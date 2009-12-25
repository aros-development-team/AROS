/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert.
    Lang: english
*/
#include <aros/config.h>
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/rawfmt.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <aros/system.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include "alertstrings.h"

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

    struct Task *task;
    UBYTE buffer[256], *buf;

    PrintFrame();
    PrintCentered(Alert_GetTitle(alertNum));
    NewRawDoFmt(fmtstring, RAWFMTFUNC_STRING, buffer, task, Alert_GetTaskName(SysBase));
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
    }
    AROS_LIBFUNC_EXIT
}
