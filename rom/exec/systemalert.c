/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert in supervisor mode.
    Lang: english

    This function can be replaced with architecture-specific implementation.
    It is called in Disable()d state and can do whatever it wants. Also it
    can wait for some input, if possible.
    One limitation: it must work no matter what. It's not adviced to Enable()
    here because of this.
*/

#include <exec/execbase.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

#define ALERT_WIDTH 80

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

    if (len > (ALERT_WIDTH - 2))
    	    len = (ALERT_WIDTH - 2);

    s = ALERT_WIDTH - 2 - len;
    
    RawPutChar('#');
    if (s & 1)
        RawPutChar(' ');
    s >>= 1;
    PrintChars(' ', s, SysBase);
    for (i = 0; i < len; i++)
        RawPutChar(str[i]);
    PrintChars(' ', s, SysBase);
    RawPutChar('#');
    RawPutChar('\n');

    return &str[len];
}

static char *PrintLeftJustified(char *str, struct ExecBase *SysBase)
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
    PrintChars(' ', s, SysBase);
    RawPutChar('#');
    RawPutChar('\n');

    return &str[len];
}

static inline void PrintFrame(struct ExecBase *SysBase)
{
    PrintChars('#', ALERT_WIDTH, SysBase);
    RawPutChar('\n');
}

/*
 * Print alert to the debug output.
 * In future we should have more intelligent handling for such a case. For
 * example we should report what was wrong after we rebooted.
 *
 * Note that we use shared buffer in SysBase for alert text.
 */
void Exec_SystemAlert(ULONG alertNum, APTR location, APTR stack, UBYTE type, APTR data, struct ExecBase *SysBase)
{
    char *buf;

    RawPutChar('\n');
    PrintFrame(SysBase);

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

    PrintFrame(SysBase);

    FormatAlertExtra(PrivExecBase(SysBase)->AlertBuffer, stack, type, data, SysBase);

    /* Skip the leading '\n' provided by FormatAlertExtra() */
    buf = &PrivExecBase(SysBase)->AlertBuffer[1];
    while (*buf)
	RawPutChar(*buf++);

    RawPutChar('\n');
    PrintFrame(SysBase);
    RawPutChar('\n');
}
