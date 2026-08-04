/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    AArch64 low-level debug character output.
*/

#include <aros/kernel.h>
#include <inttypes.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include "kernel_intern.h"

/*
 * All bug()/KrnBug output ends up here. The generic rom/kernel stub discards
 * everything, leaving the kernel silent during boot. ARMI_PutChar is the
 * bootstrap's framebuffer console (KRN_FuncPutC), ARMI_SerPutChar the PL011;
 * both are set by the platform probe, which runs before the first bug().
 */
int krnPutC(int chr, struct KernelBase *KernelBase)
{
    if (chr == 0x03)
        __arm_arosintern.ARMI_PutChar = NULL;
    else
    {
        if (__arm_arosintern.ARMI_PutChar)
            __arm_arosintern.ARMI_PutChar(chr);
        if (__arm_arosintern.ARMI_SerPutChar)
            __arm_arosintern.ARMI_SerPutChar(chr);
    }
    return 1;
}
