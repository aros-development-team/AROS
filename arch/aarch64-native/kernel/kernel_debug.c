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
 * everything, leaving the kernel silent during boot. Emit via the configured
 * ARMI_PutChar (set to the bootstrap UART putchar), with a direct PL011 write
 * as a fallback so output works even before/without ARMI_PutChar. The UART
 * device mapping is accessible at EL1.
 */
int krnPutC(int chr, struct KernelBase *KernelBase)
{
    if (chr == 0x03)
    {
        /* Convention: 0x03 disables further low-level output. */
        __arm_arosintern.ARMI_PutChar = NULL;
        return 1;
    }

    /*
     * Write the PL011 directly. ARMI_PutChar points into the bootstrap image
     * (KRN_FuncPutC tag) and emits nothing once the kernel is running, so use
     * the same raw UART the early markers use. The device page is EL1-mapped.
     */
    {
        volatile uint32_t *uart = (volatile uint32_t *)0x3f201000;
        if (chr == '\n')
        {
            while (uart[0x18 / 4] & (1 << 5)) ; /* wait for TXFF to clear */
            uart[0] = '\r';
        }
        while (uart[0x18 / 4] & (1 << 5)) ;
        uart[0] = chr;
    }

    return 1;
}
