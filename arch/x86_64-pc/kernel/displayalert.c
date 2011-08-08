#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/alerts.h>

#include <bootconsole.h>
#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>

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

static const char *PrintLeftJustified(const char *str, struct KernelBase *KernelBase)
{
    int len = linelen(str);
    int i;
    ULONG s;

    if (len > (scr_Width - 3))
    	    len = (scr_Width - 3);

    s = scr_Width - 3 - len;

    krnPutC(0xDB, KernelBase);
    krnPutC(' ', KernelBase);

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

static inline void PrintString(const char *buf, struct KernelBase *KernelBase)
{
    while (*buf)
	krnPutC(*buf++, KernelBase);
}

AROS_LH2(void, KrnDisplayAlert,
	 AROS_LHA(uint32_t, code, D0),
	 AROS_LHA(const char *, text, A0),
	 struct KernelBase *, KernelBase, 35, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * Display alert on the screen using libbootconsole.
     * Very useful for early alerts, while the display driver not started yet.
     * In this case the user gets a nice GURU. Not painted in red yet. :)
     */

    unsigned int i;

    if (scr_Type == SCR_UNKNOWN)
    {
       	/* Default alert width (for possible serial output). */
    	scr_Width = 80;
    }

    /* Make sure that the output starts from a new line */
    krnPutC('\n', KernelBase);

    PrintFrame(0xDF, KernelBase);

    /* Print first three lines (task and error) centered */
    for (i = 0; i < 3; i++)
    {
    	text = PrintCentered(text, KernelBase);
    	text++;	/* Skip a newline */
    }

    /* Empty line */
    PrintCentered("", KernelBase);

    /* The rest is left-justified */
    while (*text)
    {
    	if (*text == 0x0F)
    	{
    	    /* 0x0F ends the frame */
	    PrintFrame(0xDC, KernelBase);
    	    text++;
    	    break;
    	}
	else
	{
	    /* Print left-justified line inside frame */
    	    text = PrintLeftJustified(text, KernelBase);

    	    if (*text == '\n')
    	    	text++;
    	 }
    }

    /* Print the rest of alert text */
    PrintString(text, KernelBase);

    krnPutC('\n', KernelBase);

    if ((code & AT_DeadEnd) && (scr_Type != SCR_UNKNOWN))
    {
    	/*
    	 * If we have a framebuffer, the user have seen the alert.
    	 * Unfortunately we have no input yet, so just stop.
    	 */
    	PrintString("\nSystem halted. Reset the machine.", KernelBase);
    	for(;;);
    }

    /* Recoverable alerts don't halt the machine. They are just dropped to debug log. */
    PrintFrame(0xDC, KernelBase);

    AROS_LIBFUNC_EXIT
}
