/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Alert display for the sbi-riscv64 target.

    Prints the alert on the SBI console and returns.
*/

#include <kernel_base.h>
#include <kernel_debug.h>

#include "kernel_intern.h"

void krnDisplayAlert(const char *text, struct KernelBase *KernelBase)
{
    /*
     * Print and return, as the other ports do. What follows is the
     * caller's to decide: a recoverable alert carries on, a dead-end one
     * goes to ShutdownA(). The hart is not halted here.
     */
    krnSBIPutStr("\n*** alert ***\n");

    while (*text)
    {
        /* Alert text separates its fields with a form feed */
        krnSBIPutC((*text == 0x0F) ? '\n' : *text);
        text++;
    }

    krnSBIPutStr("\n*** end of alert ***\n");
}
