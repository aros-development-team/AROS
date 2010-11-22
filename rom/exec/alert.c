/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert.
    Lang: english
*/

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <string.h>

#include "exec_intern.h"
#include "exec_util.h"

#define ALERT_WIDTH 80

/* x86/64 kernel.resource doesn't have KrnIsSuper() */
#ifndef KrnIsSuper
#define KrnIsSuper() 0
#endif

static void PrintChars(char c, ULONG n)
{
    while (n--)
        RawPutChar(c);
}

static int linelen(char *str)
{
    int l;

    for (l = 0; str[l] && str[l] != '\n'; l++);
    return l;
}

static char *PrintCentered(char *str)
{
    int len = linelen(str);
    int i;
    ULONG s;

    if (len > (ALERT_WIDTH - 2))
    	    len = (ALERT_WIDTH - 2);

    s = ALERT_WIDTH - 2 - len;
    
    RawPutChar('#');
    if (s & 1)
        RawPutChar(' ');
    s >>= 1;
    PrintChars(' ', s);
    for (i = 0; i < len; i++)
        RawPutChar(str[i]);
    PrintChars(' ', s);
    RawPutChar('#');
    RawPutChar('\n');

    return &str[len];
}

static char *PrintLeftJustified(char *str)
{
    int len = linelen(str);
    int i;
    ULONG s;

    if (len > (ALERT_WIDTH - 3))
    	    len = (ALERT_WIDTH - 3);

    s = ALERT_WIDTH - 3 - len;

    RawPutChar('#');
    RawPutChar(' ');
    for (i = 0; i < len; i++)
        RawPutChar(str[i]);
    PrintChars(' ', s);
    RawPutChar('#');
    RawPutChar('\n');

    return &str[len];
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

    struct Task *task = SysBase->ThisTask;
    char *buf;

    /* If we are running in user mode we should first try to report a problem using AROS'
       own way to do it */
    if (!KrnIsSuper())
    {
        alertNum = Exec_UserAlert(alertNum, task, SysBase);
	if (!alertNum)
	    return;
    }

    /* 
     * We're here if Intuition failed. Print alert to the debug output and reboot.
     * In future we should have more intelligent handling for such a case. For
     * example we should report what was wrong after we rebooted.
     *
     * Note that we use shared buffer in SysBase for alert text, so we Disable()
     */
    Disable();

    PrintFrame();
    PrintCentered(Alert_GetTitle(alertNum));

    FormatAlert(PrivExecBase(SysBase)->AlertBuffer, alertNum, task, SysBase);
    /* Print first two lines (task and error) centered */
    buf = PrintCentered(PrivExecBase(SysBase)->AlertBuffer);
    buf = PrintCentered(buf + 1);
    /* The rest is left-justified */
    while (*buf)
    	buf = PrintLeftJustified(buf + 1);

    PrintFrame();
    RawPutChar('\n');

    Enable();

    if (alertNum & AT_DeadEnd)
    {
	/* Um, we have to do something here in order to prevent the
	   computer from continuing... */
	ColdReboot();
	ShutdownA(SD_ACTION_COLDREBOOT);
    }
    AROS_LIBFUNC_EXIT
}
