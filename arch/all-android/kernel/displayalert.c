/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert, Android-hosted version
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <signal.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

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

AROS_LH2(void, KrnDisplayAlert,
	 AROS_LHA(uint32_t, code, D0),
	 AROS_LHA(const char *, text, A0),
	 struct KernelBase *, KernelBase, 35, Kernel)
{
    AROS_LIBFUNC_INIT

    /*
     * This version displays an alert in Android GUI additionally to logging it.
     * We still want it in debug log because we can't read it in real time.
     * There's no usable stderr on Android, we can only redirect log into a file.
     * TODO: The code starts duplicating, this isn't good... Needs some cleanup...
     */
    const char *start = text;
    unsigned int i;

    /* Make sure that the output starts from a new line */
    krnPutC('\n', KernelBase);

    PrintFrame(KernelBase);

    /* Print first three lines (title, task and error) centered */
    for (i = 0; i < 3; i++)
    {
    	text = PrintCentered(text, KernelBase);
    	text++;	/* Skip a newline */
    }

    PrintFrame(KernelBase);

    /* Print the rest of alert text */
    while (*text)
    	krnPutC(*text++, KernelBase);

    krnPutC('\n', KernelBase);
    PrintFrame(KernelBase);

    /*
     * Explicitly disable task switching.
     * Yes, we are in Disable(). However Dalvik VM will enable SIGALRM.
     * This means Disable()d state will be broken. Additionally it messes
     * with stack or threads, which will cause AN_StackProbe guru during
     * displaying an alert if we don't do this.
     */
    Forbid();

    /* Display the alert via Java interface. */
    KernelBase->kb_PlatformData->DisplayAlert(start);

    /*
     * Fix up interrupts state before Permit().
     * Yes, there will be Enable() after return, but let's not
     * forget about nesting count.
     */
    KernelIFace.sigprocmask(SIG_BLOCK, &KernelBase->kb_PlatformData->sig_int_mask, NULL);
    Permit();

    AROS_LIBFUNC_EXIT
}
