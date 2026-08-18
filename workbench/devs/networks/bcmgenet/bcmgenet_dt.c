/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Finding the Broadcom GENETv5 controller on a BCM2711 device tree.

    Everything that knows how this platform describes its hardware lives
    here. Unlike the legacy VideoCore peripherals (RPiHDMI, RPiPWM, ...),
    which sit on the 0x7Ennnnnn bus window translated by
    BCM2711_BUS_PERIIOBASE, GENET is a child of the "scb" simple-bus and
    its "reg" is two address cells translated by scb's own "ranges" - the
    result is the well-known 0xFD580000 physical address, which is kept
    below as a fallback if that translation cannot be read for some
    reason.
*/
#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/openfirmware.h>

#include <aros/macros.h>

#include "bcmgenet.h"

#define GENET_COMPATIBLE        "brcm,bcm2711-genet-v5"
#define GENET_MDIO_COMPATIBLE   "brcm,genet-mdio-v5"

/* GICv2/v3 SPI interrupts start at INTID 32; "interrupts" cell 1 of each
 * <type, number, flags> triplet is the SPI number. See pcie_init.c for
 * the same convention on this SoC's other GIC-attached peripherals. */
#define GIC_SPI_BASE            32

/*
 * TODO: the mdio child node's own child (the actual PHY, e.g.
 * "ethernet-phy@1") carries the mdio address in its "reg" property.
 * Walk it with OF_GetChild() the way the caller already has the mdio
 * node in hand. Falls back to address 1, which is where the Pi 4B's
 * BCM54213PE sits (see &genet_mdio in bcm2711-rpi-4-b.dts).
 */
static ULONG bcmgenet_phyaddress(APTR mdio_node)
{
    return 1;
}

/*
 * TODO: read the "phy-mode" string property (e.g. "rgmii-rxid") and
 * map it to enum genet_phy_mode. Falls back to the mode wired on every
 * shipping Pi 4B.
 */
static enum genet_phy_mode bcmgenet_phymode(APTR node)
{
    return GENET_PHY_MODE_RGMII_RXID;
}

BOOL BCMGENET_Discover(struct BCMGENETBase *base, struct bcmgenet_hw *hw)
{
    APTR OpenFirmwareBase = base->bgm_OpenFirmwareBase;
    APTR node, mdio_node, prop;
    ULONG len;

    node = OF_FindNodeByCompatible(NULL, GENET_COMPATIBLE);
    if (!node)
    {
        D(bug("[bcmgenet] no \"%s\" node in the device tree\n", GENET_COMPATIBLE);)
        return FALSE;
    }

    /*
     * TODO: read "reg" (two address cells + one size cell on the scb
     * bus - NOT the single-cell layout RPiHDMI's get_device_tree_reg_value()
     * expects) and translate it through scb's "ranges". Until that is
     * written, fall back to the known Pi 4 address.
     */
    hw->phys = GENET_PHYS_BASE;
    hw->size = GENET_PHYS_SIZE;
    hw->base = hw->phys;

    /*
     * TODO: read both cells of "interrupts" (RXDMA and TXDMA are
     * separate GIC SPIs on the BCM2711 - 157 and 158). Mirror
     * pcie_init.c's raw cell parsing with AROS_BE2LONG().
     */
    hw->irq[0] = GIC_SPI_BASE + 157;
    hw->irq[1] = GIC_SPI_BASE + 158;

    mdio_node = OF_FindNodeByCompatible(node, GENET_MDIO_COMPATIBLE);
    hw->phyAddr = mdio_node ? bcmgenet_phyaddress(mdio_node) : 1;
    hw->phyMode = bcmgenet_phymode(node);

    /* Firmware sometimes leaves the address it used in the tree */
    hw->haveMacAddr = FALSE;
    prop = OF_FindProperty(node, "local-mac-address");
    if (prop && OF_GetPropLen(prop) == ETH_ADDRESSSIZE)
    {
        CopyMem(OF_GetPropValue(prop), hw->macAddr, ETH_ADDRESSSIZE);
        hw->haveMacAddr = TRUE;
    }

    D(bug("[bcmgenet] @ %p+%p, irq %u/%u, phy %u, mode %u\n",
          (APTR)hw->phys, (APTR)hw->size, hw->irq[0], hw->irq[1],
          hw->phyAddr, hw->phyMode);)

    return TRUE;
}
