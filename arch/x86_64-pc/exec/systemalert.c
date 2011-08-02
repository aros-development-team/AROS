/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: systemalert.c 38733 2011-05-18 09:32:02Z sonic $

    Desc: Display an alert in supervisor mode. Experimental version for native ports.
    Lang: english

    TODO: Currently this function relies on static linking between kernel.resource and exec.library
    because it directly looks at libbootconsole's variables. In future there should be some
    kind of KrnDisplayAlert(), however the mechanism is not developed well yet.
*/

#include <asm/cpu.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <bootconsole.h>

#include "exec_intern.h"
#include "exec_util.h"

static inline void PrintChars(char c, ULONG n, struct ExecBase *SysBase)
{
    while (n--)
        RawPutChar(c);
}

static inline int linelen(char *str)
{
    int l;

    for (l = 0; str[l] && str[l] != '\n'; l++);
    return l;
}

static char *PrintCentered(char *str, struct ExecBase *SysBase)
{
    int len = linelen(str);
    int i;
    ULONG s;

    if (len > (scr_Width - 2))
    	    len = (scr_Width - 2);

    s = scr_Width - 2 - len;

    RawPutChar(0xDB);
    if (s & 1)
        RawPutChar(' ');
    s >>= 1;
    PrintChars(' ', s, SysBase);
    for (i = 0; i < len; i++)
        RawPutChar(str[i]);
    PrintChars(' ', s, SysBase);
    RawPutChar(0xDB);
    RawPutChar('\n');

    return &str[len];
}

static char *PrintLeftJustified(char *str, struct ExecBase *SysBase)
{
    int len = linelen(str);
    int i;
    ULONG s;

    if (len > (scr_Width - 3))
    	    len = (scr_Width - 3);

    s = scr_Width - 3 - len;

    RawPutChar(0xDB);
    RawPutChar(' ');
    for (i = 0; i < len; i++)
        RawPutChar(str[i]);
    PrintChars(' ', s, SysBase);
    RawPutChar(0xDB);
    RawPutChar('\n');

    return &str[len];
}

static inline void PrintFrame(char c, struct ExecBase *SysBase)
{
    RawPutChar(0xDB);
    PrintChars(c, scr_Width - 2, SysBase);
    RawPutChar(0xDB);
    RawPutChar('\n');
}

static inline void PrintString(const char *buf, struct ExecBase *SysBase)
{
    while (*buf)
	RawPutChar(*buf++);
}

/*
 * Display alert on the screen using libbootconsole.
 * Very useful for early alerts, while the display driver not started yet.
 * In this case the user gets a nice GURU. Not painted in red yet. :)
 */
void Exec_SystemAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase)
{
    char *buf;

    if (scr_Type == SCR_UNKNOWN)
    {
       	/* Default alert width (for possible serial output). */
    	scr_Width = 80;
    }

    RawPutChar('\n');
    PrintFrame(0xDF, SysBase);

    /* Print alert title centered */
    PrintCentered(Alert_GetTitle(alertNum), SysBase);

    /* Get the alert text */
    FormatAlert(PrivExecBase(SysBase)->AlertBuffer, alertNum, SysBase->ThisTask, location, type, SysBase);

    /* Print first two lines (task and error) centered */
    buf = PrintCentered(PrivExecBase(SysBase)->AlertBuffer, SysBase);
    buf = PrintCentered(buf + 1, SysBase);

    /* Empty line */
    PrintCentered("", SysBase);

    /* The rest is left-justified */
    while (*buf)
    	buf = PrintLeftJustified(buf + 1, SysBase);

    PrintFrame(0xDC, SysBase);

    FormatAlertExtra(PrivExecBase(SysBase)->AlertBuffer, stack, type, data, SysBase);

    /* Skip the leading '\n' provided by FormatAlertExtra() */
    PrintString(&PrivExecBase(SysBase)->AlertBuffer[1], SysBase);

    RawPutChar('\n');
    RawPutChar('\n');

    if ((alertNum & AT_DeadEnd) && (scr_Type != SCR_UNKNOWN))
    {
    	/*
    	 * If we have a framebuffer, the user have seen the alert.
    	 * Unfortunately we have no input yet, so just stop.
    	 */
    	PrintString("System halted. Reset the machine.", SysBase);
    	for(;;);
    }
}
