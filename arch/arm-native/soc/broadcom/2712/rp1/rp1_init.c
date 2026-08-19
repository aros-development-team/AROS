/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    RP1 Southbridge driver for Raspberry Pi 5
    Discovers RP1 via PCIe ECAM config space read (bus 1, dev 0).
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/macros.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "rp1.h"

#include LC_LIBDEFS_FILE

/* PCIe ECAM access for BCM2711/2712 */
#define PCIE_RC_BASE        0xFD500000
#define PCIE_EXT_CFG_INDEX  0x9000
#define PCIE_EXT_CFG_DATA   0x8000
#define PCIE_ECAM_OFFSET(bus, dev, func) \
    (((bus) << 20) | ((dev) << 15) | ((func) << 12))

static ULONG pcie_cfg_read(ULONG bus, ULONG dev, ULONG func, ULONG reg)
{
    volatile ULONG *rc = (volatile ULONG *)PCIE_RC_BASE;

    /* Write ECAM index */
    rc[PCIE_EXT_CFG_INDEX / 4] = AROS_LONG2LE(PCIE_ECAM_OFFSET(bus, dev, func));

    /* Read from data window */
    return AROS_LE2LONG(*(volatile ULONG *)(PCIE_RC_BASE + PCIE_EXT_CFG_DATA + reg));
}

static int RP1_Init(LIBBASETYPEPTR LIBBASE)
{
    ULONG id, bar1;
    UWORD vendor, device;

    D(bug("[RP1] Init — probing PCIe bus 1 for RP1\n"));

    /* Read vendor/device ID from bus 1, dev 0, func 0 */
    id = pcie_cfg_read(1, 0, 0, 0x00);
    vendor = id & 0xFFFF;
    device = (id >> 16) & 0xFFFF;

    if (vendor != RP1_PCIE_VENDOR_ID || device != RP1_PCIE_DEVICE_ID) {
        D(bug("[RP1] No RP1 found (id=0x%08lx) — not RPi5\n", id));
        return TRUE; /* Not fatal */
    }

    /* Read BAR1 (memory-mapped, 64-bit capable) */
    bar1 = pcie_cfg_read(1, 0, 0, 0x14) & ~0xF;

    if (bar1 == 0 || bar1 == 0xFFFFFFFF) {
        D(bug("[RP1] BAR1 not assigned (0x%08lx) — PCIe RC not initialized?\n", bar1));
        return TRUE;
    }

    D(bug("[RP1] Found RP1: vendor=0x%04x device=0x%04x BAR1=0x%08lx\n",
          vendor, device, bar1));

    LIBBASE->rp1_BAR1 = (IPTR)bar1;
    LIBBASE->rp1_Present = TRUE;

    /* Export peripheral addresses */
    LIBBASE->rp1_USB0  = LIBBASE->rp1_BAR1 + RP1_USB0_OFFSET;
    LIBBASE->rp1_USB1  = LIBBASE->rp1_BAR1 + RP1_USB1_OFFSET;
    LIBBASE->rp1_ETH   = LIBBASE->rp1_BAR1 + RP1_ETH_OFFSET;
    LIBBASE->rp1_GPIO  = LIBBASE->rp1_BAR1 + RP1_GPIO_OFFSET;
    LIBBASE->rp1_I2C0  = LIBBASE->rp1_BAR1 + RP1_I2C0_OFFSET;
    LIBBASE->rp1_I2C1  = LIBBASE->rp1_BAR1 + RP1_I2C1_OFFSET;
    LIBBASE->rp1_UART0 = LIBBASE->rp1_BAR1 + RP1_UART0_OFFSET;

    D(bug("[RP1] USB0=0x%p ETH=0x%p GPIO=0x%p I2C1=0x%p\n",
          LIBBASE->rp1_USB0, LIBBASE->rp1_ETH,
          LIBBASE->rp1_GPIO, LIBBASE->rp1_I2C1));

    return TRUE;
}

ADD2INITLIB(RP1_Init, 0)
