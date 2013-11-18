/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/config.h>
#include <asm/cpu.h>

#include <stdio.h>

#include "kernel_base.h"
#include "kernel_debug.h"

/*
 * This definition allows to move this buffer away from .bss.
 * Can be used on native, by using, for example, zero page.
 * This condition is considered fatal, there's no return.
 */
#ifndef KERNEL_PANIC_BUFFER
static char panicBuffer[1024];
#define KERNEL_PANIC_BUFFER panicBuffer
#endif

void krnPanic(struct KernelBase *KernelBase,const char *fmt, ...)
{
    const char *hdr = "Critical boot failure\n";
    char *ptr = KERNEL_PANIC_BUFFER;
    va_list ap;

    /* Prepend the header */
    while (*hdr)
	*ptr++ = *hdr++;

    /* vsprintf() here comes from librom.a */
    va_start(ap, fmt);
    vsprintf(ptr, fmt, ap);
    va_end(ap);

    krnDisplayAlert(KERNEL_PANIC_BUFFER, KernelBase);

#if AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE
    /* Hosted AROS may quit here */
    for (;;) HALT;
#endif
}
