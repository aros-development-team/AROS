/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    RP1 USB 3.0 (xHCI) platform initialization for Raspberry Pi 5
    Discovers xHCI controllers from rp1.resource and exports MMIO addresses.
*/

/* Bring-up diagnostics: xHCI bases and the CAPLENGTH probe. */
#define DEBUG 1

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <hardware/bcm2708.h>

#include "rp1.h"

#include LC_LIBDEFS_FILE

APTR KernelBase = NULL;

static int RP1USB_Init(LIBBASETYPEPTR LIBBASE)
{
    struct RP1Base *rp1base;

    D(bug("[RP1-USB] Init\n"));

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return TRUE;

    /* Only active on BCM2712 */
    if ((IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) != BCM2712_PERIIOBASE) {
        D(bug("[RP1-USB] Only used on BCM2712, skipping\n"));
        return TRUE;
    }

    /* Get RP1 BAR1 from rp1.resource */
    rp1base = OpenResource("rp1.resource");
    if (!rp1base) {
        D(bug("[RP1-USB] rp1.resource not available\n"));
        return TRUE;
    }

    if (!rp1base->rp1_Present) {
        D(bug("[RP1-USB] RP1 not present\n"));
        return TRUE;
    }
    
    LIBBASE->usb0_base = rp1base->rp1_USB0;
    LIBBASE->usb1_base = rp1base->rp1_USB1;
    LIBBASE->dma_offset = rp1base->rp1_DMAOffset;
    LIBBASE->irq0 = rp1base->rp1_USBIrq0;
    LIBBASE->irq1 = rp1base->rp1_USBIrq1;
    LIBBASE->usb0_present = TRUE;
    LIBBASE->usb1_present = TRUE;

    D(bug("[RP1-USB] DMA offset 0x%p, irqs %u/%u\n", (APTR)LIBBASE->dma_offset,
          (unsigned)LIBBASE->irq0, (unsigned)LIBBASE->irq1));

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
            LIBBASE->usb0_present = FALSE;
            return TRUE;
        }
        D(bug("[RP1-USB] USB0 CAPLENGTH=0x%02x — xHCI detected\n", caplength));

        caplength = *(volatile UBYTE *)LIBBASE->usb1_base;
        if (caplength < 0x10 || caplength > 0x80) {
            D(bug("[RP1-USB] USB1 CAPLENGTH=0x%02x — invalid, xHCI not ready\n", caplength));
            LIBBASE->usb1_present = FALSE;
            return TRUE;
        }
        D(bug("[RP1-USB] USB1 CAPLENGTH=0x%02x — xHCI detected\n", caplength));
    }

    return TRUE;
}

/*
 * MMIO base of xHCI controller `unit`, or 0 if this board has no RP1 or the
 * unit does not exist.  Callers need no view of the library base.
 */
AROS_LH1(IPTR, RP1USBGetBase,
        AROS_LHA(ULONG, unit, D0),
        struct RP1USBBase *, RP1USBBase, 1, Rp1usb)
{
    AROS_LIBFUNC_INIT

    switch (unit)
    {
        case 0:
            return RP1USBBase->usb0_present ? RP1USBBase->usb0_base : 0;
        case 1:
            return RP1USBBase->usb1_present ? RP1USBBase->usb1_base : 0;
    }

    return 0;

    AROS_LIBFUNC_EXIT
}

/* Added to a CPU address to get the bus address a master must use. */
AROS_LH0(IPTR, RP1USBGetDMAOffset,
        struct RP1USBBase *, RP1USBBase, 2, Rp1usb)
{
    AROS_LIBFUNC_INIT

    return RP1USBBase->dma_offset;

    AROS_LIBFUNC_EXIT
}

/* GIC INTID controller `unit` signals on, 0 if MSI is not up. */
AROS_LH1(ULONG, RP1USBGetIRQ,
        AROS_LHA(ULONG, unit, D0),
        struct RP1USBBase *, RP1USBBase, 3, Rp1usb)
{
    AROS_LIBFUNC_INIT

    switch (unit)
    {
        case 0:
            return RP1USBBase->usb0_present ? RP1USBBase->irq0 : 0;
        case 1:
            return RP1USBBase->usb1_present ? RP1USBBase->irq1 : 0;
    }

    return 0;

    AROS_LIBFUNC_EXIT
}

ADD2INITLIB(RP1USB_Init, 0)
