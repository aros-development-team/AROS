/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Alert display for the opensbi-riscv64 target.

    A failure that repeats scrolls the first one out of the console
    before it can be read, and the first one is the one worth having.
    Print it, say the machine is stopping, and stop.
*/

#include <kernel_base.h>
#include <kernel_debug.h>

#include "kernel_intern.h"

void krnDisplayAlert(const char *text, struct KernelBase *KernelBase)
{
    static int reported;

    if (reported)
        return;
    reported = 1;

    krnSBIPutStr("\n*** alert ***\n");

    while (*text)
    {
        /* Alert text separates its fields with a form feed */
        krnSBIPutC((*text == 0x0F) ? '\n' : *text);
        text++;
    }

    krnSBIPutStr("\n*** halted on the first alert ***\n");

    for (;;)
        asm volatile("wfi");
}
