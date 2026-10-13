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
#define DEBUG 0
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
 * The mdio node's own child (e.g. "ethernet-phy@1") carries the address on
 * the mdio bus in its "reg" property. Falls back to 1, which is where the
 * Pi 4B's BCM54213PE sits (see &genet_mdio in bcm2711-rpi-4-b.dts).
 */
static ULONG bcmgenet_phyaddress(APTR OpenFirmwareBase, APTR mdio_node)
{
    APTR phy_node, prop;

    phy_node = OF_GetChild(mdio_node, NULL);
    if (phy_node)
    {
        prop = OF_FindProperty(phy_node, "reg");
        if (prop && OF_GetPropLen(prop) >= sizeof(ULONG))
            return AROS_BE2LONG(*(const ULONG *)OF_GetPropValue(prop)) & 0x1f;
    }

    D(bug("[bcmgenet] no phy address in the tree, assuming 1\n");)
    return 1;
}

/* Both sides NUL-terminated. Local rather than strcmp(): a driver reached
 * through a NULL StdCBase would fault on the libc one. */
static BOOL genet_streq(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b)
    {
        a++;
        b++;
    }

    return (*a == '\0' && *b == '\0');
}

/*
 * "phy-mode" decides which end of the link supplies the RGMII clock delays,
 * and getting it wrong costs one direction of traffic - so it is read rather
 * than assumed, even though every shipping Pi 4B says "rgmii-rxid".
 */
static enum genet_phy_mode bcmgenet_phymode(APTR OpenFirmwareBase, APTR node)
{
    static const struct
    {
        const char         *name;
        enum genet_phy_mode mode;
    } modes[] =
    {
        { "rgmii-rxid", GENET_PHY_MODE_RGMII_RXID },
        { "rgmii-txid", GENET_PHY_MODE_RGMII_TXID },
        { "rgmii-id",   GENET_PHY_MODE_RGMII_ID   },
        { "rgmii",      GENET_PHY_MODE_RGMII      },
    };
    APTR prop;
    const char *value;
    ULONG len, i;

    prop = OF_FindProperty(node, "phy-mode");
    if (prop && (len = OF_GetPropLen(prop)) > 0)
    {
        value = OF_GetPropValue(prop);

        /* A string property carries its terminator; without one it is not
         * one, and walking it would run off the end of the property. */
        if (value[len - 1] == '\0')
        {
            for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
            {
                if (genet_streq(value, modes[i].name))
                    return modes[i].mode;
            }

            D(bug("[bcmgenet] unknown phy-mode \"%s\"\n", value);)
        }
    }

    return GENET_PHY_MODE_RGMII_RXID;
}

struct genet_cells
{
    ULONG child_ac;
    ULONG child_sc;
    ULONG parent_ac;
};

static ULONG genet_cellcount(APTR OpenFirmwareBase, APTR node, char *name)
{
    APTR prop = OF_FindProperty(node, name);

    if (prop && OF_GetPropLen(prop) >= sizeof(ULONG))
        return AROS_BE2LONG(*(const ULONG *)OF_GetPropValue(prop));

    return 1;
}

static void genet_getcells(APTR OpenFirmwareBase, struct genet_cells *c)
{
    APTR node;

    c->child_ac = 1;
    c->child_sc = 1;
    c->parent_ac = 1;

    node = OF_OpenKey("/scb");
    if (node)
    {
        c->child_ac = genet_cellcount(OpenFirmwareBase, node, "#address-cells");
        c->child_sc = genet_cellcount(OpenFirmwareBase, node, "#size-cells");
        OF_CloseKey(node);
    }

    node = OF_OpenKey("/");
    if (node)
    {
        c->parent_ac = genet_cellcount(OpenFirmwareBase, node, "#address-cells");
        OF_CloseKey(node);
    }
}

/* Consume 'count' big-endian cells, most significant first */
static UQUAD genet_readcells(const ULONG **cells, ULONG count)
{
    UQUAD val = 0;

    while (count--)
        val = (val << 32) | AROS_BE2LONG(*(*cells)++);

    return val;
}

static BOOL genet_reg(APTR OpenFirmwareBase, APTR node,
                      const struct genet_cells *c,
                      UQUAD *busaddr, UQUAD *size)
{
    APTR prop;
    const ULONG *cells;
    ULONG len;

    prop = OF_FindProperty(node, "reg");
    if (!prop)
    {
        D(bug("[bcmgenet] GENET node has no reg property\n");)
        return FALSE;
    }

    len = OF_GetPropLen(prop);
    D(bug("[bcmgenet] GENET reg length = %lu\n", len);)

    if (len < (c->child_ac + c->child_sc) * sizeof(ULONG))
    {
        D(bug("[bcmgenet] GENET reg is too short\n");)
        return FALSE;
    }

    cells = OF_GetPropValue(prop);

    *busaddr = genet_readcells(&cells, c->child_ac);
    *size = genet_readcells(&cells, c->child_sc);

    return TRUE;
}
static BOOL bcmgenet_translate_scb(APTR OpenFirmwareBase,
                                   const struct genet_cells *c,
                                   UQUAD busaddr, UQUAD size,
                                   IPTR *phys)
{
    APTR scb, prop;
    const ULONG *cells;
    ULONG entry_cells = c->child_ac + c->parent_ac + c->child_sc;
    LONG count;
    BOOL found = FALSE;

    scb = OF_OpenKey("/scb");
    if (!scb)
    {
        D(bug("[bcmgenet] no /scb node\n");)
        return FALSE;
    }

    prop = OF_FindProperty(scb, "ranges");
    if (!prop)
    {
        D(bug("[bcmgenet] /scb has no ranges property\n");)
        OF_CloseKey(scb);
        return FALSE;
    }

    cells = OF_GetPropValue(prop);
    count = OF_GetPropLen(prop) / sizeof(*cells);

    while (count >= (LONG)entry_cells)
    {
        UQUAD child_base;
        UQUAD parent_base;
        UQUAD range_size;
        UQUAD offset;

        child_base = genet_readcells(&cells, c->child_ac);
        parent_base = genet_readcells(&cells, c->parent_ac);
        range_size = genet_readcells(&cells, c->child_sc);

        count -= entry_cells;

        if (busaddr < child_base)
            continue;

        offset = busaddr - child_base;

        /* The complete GENET register window must fit in this range. */
        if (offset <= range_size && size <= range_size - offset)
        {
            *phys = (IPTR)(parent_base + offset);
            found = TRUE;
            break;
        }
    }

    OF_CloseKey(scb);
    return found;
}


BOOL BCMGENET_Discover(struct BCMGENETBase *base, struct bcmgenet_hw *hw)
{
    APTR OpenFirmwareBase = base->bgm_OpenFirmwareBase;
    APTR node, mdio_node, prop;
    struct genet_cells cells;
    UQUAD busaddr, size;

    node = OF_FindNodeByCompatible(NULL, GENET_COMPATIBLE);
    if (!node) {
        D(bug("[bcmgenet] no \"%s\" node in the device tree\n", GENET_COMPATIBLE);)
        return FALSE;
    }

    genet_getcells(OpenFirmwareBase, &cells);

    if (genet_reg(OpenFirmwareBase, node, &cells, &busaddr, &size) &&
        bcmgenet_translate_scb(OpenFirmwareBase, &cells, busaddr, size,
                               &hw->phys)) {
        hw->size = (IPTR)size;
        hw->base = hw->phys;

        D(bug("[bcmgenet] DT translated %08lx:%08lx -> %p+%p\n",
              (ULONG)(busaddr >> 32), (ULONG)busaddr,
              (APTR)hw->phys, (APTR)hw->size);)
    } else {
        D(bug("Failed to find reg property values\n");)
        hw->phys = GENET_PHYS_BASE;
        hw->size = GENET_PHYS_SIZE;
        hw->base = hw->phys;
    }

    /*
     * "interrupts" is a list of <type, number, flags> triplets; cell 1 of
     * each is the SPI number. GENET raises two, INTRL2_0 and INTRL2_1 (157
     * and 158 on the BCM2711), and only the first carries the default
     * queue's TX/RX completions this driver waits on.
     */
    hw->irq[0] = GIC_SPI_BASE + 157;
    hw->irq[1] = GIC_SPI_BASE + 158;

    prop = OF_FindProperty(node, "interrupts");
    if (prop && OF_GetPropLen(prop) >= 6 * sizeof(ULONG))
    {
        const ULONG *cells = OF_GetPropValue(prop);

        hw->irq[0] = GIC_SPI_BASE + AROS_BE2LONG(cells[1]);
        hw->irq[1] = GIC_SPI_BASE + AROS_BE2LONG(cells[4]);
    }
    else
    {
        D(bug("[bcmgenet] no usable interrupts property, assuming 157/158\n");)
    }

    mdio_node = OF_FindNodeByCompatible(NULL, GENET_MDIO_COMPATIBLE);
    hw->phyAddr = mdio_node ?
        bcmgenet_phyaddress(OpenFirmwareBase, mdio_node) : 1;
    hw->phyMode = bcmgenet_phymode(OpenFirmwareBase, node);

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
