/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    RP1 USB 3.0 (xHCI) platform initialization for Raspberry Pi 5
    Discovers xHCI controllers from rp1.resource and exports MMIO addresses.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "rp1.h"

#include LC_LIBDEFS_FILE

APTR KernelBase = NULL;

static int RP1USB_Init(LIBBASETYPEPTR LIBBASE)
{
    struct Library *rp1base;
    IPTR *fields;
    IPTR bar1;

    D(bug("[RP1-USB] Init\n"));

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return TRUE;

    /* Only active on RPi5 */
    if ((IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) == 0xFE000000) {
        D(bug("[RP1-USB] RPi4 — using VL805 via PCIe instead\n"));
        return TRUE;
    }

    /* Get RP1 BAR1 from rp1.resource */
    rp1base = OpenResource("rp1.resource");
    if (!rp1base) {
        D(bug("[RP1-USB] rp1.resource not available\n"));
        return TRUE;
    }

    fields = (IPTR *)((UBYTE *)rp1base + sizeof(struct Library));
    if (!fields[0]) {
        D(bug("[RP1-USB] RP1 not present\n"));
        return TRUE;
    }

    bar1 = fields[1];
    LIBBASE->usb0_base = bar1 + RP1_USB0_OFFSET;
    LIBBASE->usb1_base = bar1 + RP1_USB1_OFFSET;
    LIBBASE->present = TRUE;

    D(bug("[RP1-USB] xHCI USB0 at 0x%p\n", LIBBASE->usb0_base));
    D(bug("[RP1-USB] xHCI USB1 at 0x%p\n", LIBBASE->usb1_base));

    /*
     * Verify xHCI presence by reading CAPLENGTH register.
     * A valid xHCI controller has CAPLENGTH in range 0x10-0x80.
     */
    {
        UBYTE caplength = *(volatile UBYTE *)LIBBASE->usb0_base;
        if (caplength < 0x10 || caplength > 0x80) {
            D(bug("[RP1-USB] USB0 CAPLENGTH=0x%02x — invalid, xHCI not ready\n", caplength));
            LIBBASE->present = FALSE;
            return TRUE;
        }
        D(bug("[RP1-USB] USB0 CAPLENGTH=0x%02x — xHCI detected\n", caplength));
    }

    return TRUE;
}

ADD2INITLIB(RP1USB_Init, 0)
