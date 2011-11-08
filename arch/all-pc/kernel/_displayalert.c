#include <bootconsole.h>
#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include "alert_arch.h"

/*
 * Display alert on the screen using libbootconsole.
 * Very useful for early alerts, while the display driver not started yet.
 * In this case the user gets a nice GURU. Not painted in red yet. :)
 */

static inline void PrintChars(char c, ULONG n, struct KernelBase *KernelBase)
{
    while (n--)
        krnPutC(c, KernelBase);
}

/*
 * This function calculates length of line.
 * It's similar to strlen(), but stops also at LF and FF codes.
 */
static inline int linelen(const char *str)
{
    int l;

    for (l = 0; str[l] && str[l] != '\n' && str[l] != 0x0F; l++);
    return l;
}

static const char *PrintCentered(const char *str, struct KernelBase *KernelBase)
{
    int len = linelen(str);
    int i;
    ULONG s;

    if (len > (scr_Width - 2))
    	    len = (scr_Width - 2);

    s = scr_Width - 2 - len;

    krnPutC(0xDB, KernelBase);
    if (s & 1)
        krnPutC(' ', KernelBase);
    s >>= 1;
    PrintChars(' ', s, KernelBase);

    for (i = 0; i < len; i++)
        krnPutC(*str++, KernelBase);

    PrintChars(' ', s, KernelBase);
    krnPutC(0xDB, KernelBase);
    krnPutC('\n', KernelBase);

    return str;
}

static inline void PrintFrame(char c, struct KernelBase *KernelBase)
{
    krnPutC(0xDB, KernelBase);
    PrintChars(c, scr_Width - 2, KernelBase);
    krnPutC(0xDB, KernelBase);
    krnPutC('\n', KernelBase);
}

void krnDisplayAlert(const char *text, struct KernelBase *KernelBase)
{
    unsigned int i;

    if (scr_Type == SCR_UNKNOWN)
    {
       	/* Default alert width (for possible serial output). */
    	scr_Width = 80;
    }

    /* Make sure that the output starts from a new line */
    krnPutC('\n', KernelBase);

    PrintFrame(0xDF, KernelBase);

    /* Print first three lines (title, task and error) centered */
    for (i = 0; i < 3; i++)
    {
    	text = PrintCentered(text, KernelBase);
    	if (*text == 0)	/* Handle early NULL terminator */
    	    break;
    	text++;	/* Skip a newline */
    }

    PrintFrame(0xDC, KernelBase);

    /* Print the rest of alert text (if any) */
    if (*text)
    {
    	PrintString(text, KernelBase);

	/* Print a line in the bottom */
	krnPutC('\n', KernelBase);
	PrintChars(0xDC, scr_Width, KernelBase);
	krnPutC('\n', KernelBase);
    }
}
