/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/config.h>
#include <asm/cpu.h>
#include <exec/alerts.h>

#include <stdio.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_android.h"

static char panicBuffer[1024];

void krnPanic(struct KernelBase *KernelBase, const char *fmt, ...)
{
    const char *hdr = "Critical boot failure\n";
    char *ptr = panicBuffer;
    va_list ap;

    /* Prepend the header */
    while (*hdr)
	*ptr++ = *hdr++;

    /* vsprintf() here comes from librom.a */
    va_start(ap, fmt);
    vsprintf(ptr, fmt, ap);
    va_end(ap);

    if (alertPipe == -1)
    {
    	/* Very early panic (failure to load host's libc). Just dump to debug output. */
    	bug(panicBuffer);
    	return;
    }

    /*
     * Alert code is used by display server to specify buttons set in the dialog.
     * We signal it's a deadend.
     */
    SendAlert(AT_DeadEnd, panicBuffer);

    /* We simply return here, allowing the process to exit. */
}
