/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <kernel_base.h>
#include <kernel_debug.h>

#define ALERT_WIDTH 80

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

    if (len > (ALERT_WIDTH - 2))
    	    len = (ALERT_WIDTH - 2);

    s = ALERT_WIDTH - 2 - len;

    krnPutC('#', KernelBase);
    if (s & 1)
        krnPutC(' ', KernelBase);
    s >>= 1;
    PrintChars(' ', s, KernelBase);

    for (i = 0; i < len; i++)
        krnPutC(*str++, KernelBase);

    PrintChars(' ', s, KernelBase);
    krnPutC('#', KernelBase);
    krnPutC('\n', KernelBase);

    return str;
}

static inline void PrintFrame(struct KernelBase *KernelBase)
{
    PrintChars('#', ALERT_WIDTH, KernelBase);
    krnPutC('\n', KernelBase);
}

/*
 * This entry point just displays the message. It doesn't make any decisions
 * what to do next.
 * Because of this it misses 'code' argument.
 * The kernel can use it internally, for displaying messages about
 * critical startup failures.
 */
void krnDisplayAlert(const char *text, struct KernelBase *KernelBase)
{
    unsigned int i;

    /* Make sure that the output starts from a new line */
    krnPutC('\n', KernelBase);

    PrintFrame(KernelBase);

    /* Print first three lines (title, task and error) centered */
    for (i = 0; i < 3; i++)
    {
    	text = PrintCentered(text, KernelBase);

    	if (*text == 0)	/* Handle early NULL terminator */
    	    break;

    	text++;	/* Skip a newline */
    }

    PrintFrame(KernelBase);

    /* Print the rest of alert text (if any) */
    if (*text)
    {
    	while (*text)
    	    krnPutC(*text++, KernelBase);

    	krnPutC('\n', KernelBase);
    	PrintFrame(KernelBase);
    }
}
