/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Finding a DesignWare MAC on a machine described by a device tree.

    Everything that knows how this platform describes its hardware lives
    here. A machine offering openfirmware.resource could answer the same
    questions through OF_FindProperty without the rest of the driver
    noticing.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <aros/kernel.h>
#include <utility/tagitem.h>

#include <fdt.h>

#include "dwmac.h"

/*
 * The parts this driver can drive. A 3.x core answers to "snps,dwmac"
 * as well but has a different register map, so only the generations
 * sharing the 4.x/5.x layout are listed.
 */
static CONST_STRPTR const dwmac_compatible[] =
{
    "snps,dwmac-5.10a",
    "snps,dwmac-5.00a",
    "snps,dwmac-4.20a",
    "snps,dwmac-4.10a",
    "snps,dwmac-4.00",
    NULL
};

static fdt_node_t dwmac_findphandle(ULONG phandle)
{
    fdt_node_t n;

    for (n = FDT_Root(); n != FDT_NONE; n = FDT_NextNode(n))
    {
        if (FDT_GetPropU32(n, "phandle", 0) == phandle)
            return n;
    }
    return FDT_NONE;
}

/*
 * The mdio divider is derived from the clock feeding the block's control
 * registers, so the right one is the clock named "stmmaceth" - the name
 * the binding uses for it whatever the SoC calls its clock nodes.
 */
static ULONG dwmac_csrclock(fdt_node_t node)
{
    const ULONG *clocks;
    CONST_STRPTR names;
    ULONG len = 0, nlen = 0, index = 0, i;

    names = FDT_GetProp(node, "clock-names", &nlen);
    if (names)
    {
        ULONG off = 0;

        for (i = 0; off < nlen; i++)
        {
            CONST_STRPTR nm = &names[off];

            if (nm[0] == 's' && nm[1] == 't' && nm[2] == 'm')
            {
                index = i;
                break;
            }
            while ((off < nlen) && names[off])
                off++;
            off++;
        }
    }

    clocks = FDT_GetProp(node, "clocks", &len);
    if (clocks && (len >= (index + 1) * 4))
    {
        fdt_node_t src = dwmac_findphandle(
                             (ULONG)FDT_ReadCells(&clocks[index], 1));

        if (src != FDT_NONE)
            return FDT_GetPropU32(src, "clock-frequency", 0);
    }

    return 0;
}

/*
 * The PHY hangs off an mdio child node; its "reg" is the address to
 * talk to it on. Absent that, address 0 is the usual place and probing
 * will find out soon enough.
 */
static ULONG dwmac_phyaddress(fdt_node_t node)
{
    fdt_node_t n;
    ULONG depth = 0;

    for (n = FDT_NextNode(node); n != FDT_NONE; n = FDT_NextNode(n))
    {
        UQUAD addr = 0, size = 0;

        if (FDT_HasProp(n, "compatible") &&
            FDT_IsCompatible(n, "snps,dwmac-mdio"))
        {
            depth = 1;
            continue;
        }

        if (depth && FDT_HasProp(n, "reg") &&
            FDT_GetReg(n, 0, &addr, &size))
            return (ULONG)addr;

        if (depth)
            break;
    }

    return 0;
}

static BOOL dwmac_enabled(fdt_node_t node)
{
    CONST_STRPTR status = FDT_GetPropStr(node, "status");

    /* Absent means usable; anything but "ok"/"okay" means it is not */
    if (!status)
        return TRUE;

    return (status[0] == 'o' && status[1] == 'k');
}

#undef UtilityBase

BOOL DWMAC_Discover(struct DWMACBase *base, struct dwmac_hw *hw)
{
    APTR KernelBase = base->dwm_KernelBase;
    struct Library *UtilityBase = base->dwm_UtilityBase;
    fdt_node_t node = FDT_NONE;
    UQUAD addr = 0, size = 0;
    const ULONG *prop;
    ULONG len = 0, i;
    APTR dtb, bootinfo;

    bootinfo = KrnGetBootInfo();
    dtb = (APTR)GetTagData(KRN_FlattenedDeviceTree, 0,
                           (struct TagItem *)bootinfo);
    if (!dtb || !FDT_Open(dtb))
    {
        D(bug("[dwmac] no device tree to search\n");)
        return FALSE;
    }

    for (i = 0; dwmac_compatible[i]; i++)
    {
        node = FDT_FindCompatible(FDT_Root(), dwmac_compatible[i]);
        if (node != FDT_NONE)
            break;
    }

    if (node == FDT_NONE)
    {
        D(bug("[dwmac] no controller of a generation this drives\n");)
        return FALSE;
    }

    if (!dwmac_enabled(node))
    {
        D(bug("[dwmac] %s is disabled\n", FDT_NodeName(node));)
        return FALSE;
    }

    if (!FDT_GetReg(node, 0, &addr, &size) || !addr || !size)
    {
        D(bug("[dwmac] %s has no registers\n", FDT_NodeName(node));)
        return FALSE;
    }

    hw->phys = (IPTR)addr;
    hw->size = (IPTR)size;

    /* First cell of "interrupts" is the source at the controller */
    hw->irq = 0;
    prop = FDT_GetProp(node, "interrupts", &len);
    if (prop && len >= 4)
        hw->irq = (ULONG)FDT_ReadCells(&prop[0], 1);

    hw->phyAddr = dwmac_phyaddress(node);
    hw->csrClock = dwmac_csrclock(node);

    /* Firmware sometimes leaves the address it used in the tree */
    hw->haveMacAddr = FALSE;
    prop = FDT_GetProp(node, "local-mac-address", &len);
    if (prop && len == 6)
    {
        CopyMem((APTR)prop, hw->macAddr, 6);
        hw->haveMacAddr = TRUE;
    }

    /*
     * The boot page tables cover RAM and nothing else, so the register
     * block has to be mapped before it can be touched at all. It is
     * mapped where it already lives, so the two addresses are the same.
     */
    if (!KrnMapGlobal((APTR)hw->phys, (APTR)hw->phys, (ULONG)hw->size,
                      MAP_Readable | MAP_Writable))
    {
        D(bug("[dwmac] could not map %p+%p\n", (APTR)hw->phys,
              (APTR)hw->size);)
        return FALSE;
    }

    hw->base = hw->phys;

    D(bug("[dwmac] %s @ %p+%p (mapped %p), irq %u, phy %u, csr %uHz\n",
          FDT_NodeName(node), (APTR)hw->phys, (APTR)hw->size,
          (APTR)hw->base, hw->irq, hw->phyAddr, hw->csrClock);)

    return TRUE;
}
