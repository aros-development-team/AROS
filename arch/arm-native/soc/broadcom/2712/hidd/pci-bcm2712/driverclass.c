/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 (Raspberry Pi 5) PCIe Host Bridge OOP Driver Class.
*/

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#define __NOLIBBASE__
#include <proto/kernel.h>
#define KernelBase (PSD(cl)->kernelBase)

#include <aros/symbolsets.h>
#include <string.h>
#include <hardware/pci.h>

#include "pcie2712.h"
#include <aros/debug.h>

#undef HiddPCIDriverAttrBase
#undef HiddPCIDeviceAttrBase
#undef HiddAttrBase

#define HiddPCIDriverAttrBase (PSD(cl)->hiddPCIDriverAB)
#define HiddPCIDeviceAttrBase (PSD(cl)->hiddPCIDeviceAB)
#define HiddAttrBase          (PSD(cl)->hiddAB)

OOP_Object *PCIBcm2712__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;

    struct TagItem mytags[] = {
        { aHidd_Name, (IPTR)"PCINative" },
        { aHidd_HardwareName, (IPTR)"BCM2712 PCIe host bridge (RPi5 NVMe M.2)" },
        { TAG_DONE, 0 }
    };

    mymsg.mID = msg->mID;
    mymsg.attrList = (struct TagItem *)&mytags;

    if (msg->attrList)
    {
        mytags[2].ti_Tag = TAG_MORE;
        mytags[2].ti_Data = (IPTR)msg->attrList;
    }

    msg = &mymsg;

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID PCIBcm2712__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_PCIDRV_ATTR(msg->attrID, idx) && (idx == aoHidd_PCIDriver_DeviceClass))
    {
        *msg->storage = (IPTR)PSD(cl)->deviceClass;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*
 * ECAM Configuration Space Access:
 * Bus 0 = Root Complex Header (DBI memory mapped or ECAM)
 * Bus 1 = Endpoint (e.g. NVMe SSD attached to M.2 HAT)
 */
static inline uint32_t ecam_offset(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    return (((uint32_t)bus << 20) | ((uint32_t)dev << 15) | ((uint32_t)sub << 12) | (reg & 0xFFF));
}

static ULONG ReadConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    if (!psd->link_up && bus > 0)
        return 0xFFFFFFFF;

    /* Bus 0 (RC) or Bus 1 (Endpoint) via ECAM window */
    if (psd->ecam)
    {
        uint32_t off = ecam_offset(bus, dev, sub, reg & ~3);
        if (off < BCM2712_PCIE0_ECAM_SIZE)
        {
            volatile uint32_t *p = (volatile uint32_t *)(psd->ecam + off);
            return AROS_LE2LONG(*p);
        }
    }

    /* Fallback via DBI index window */
    if (psd->regs)
    {
        volatile uint32_t *rc = (volatile uint32_t *)psd->regs;
        rc[PCIE2712_EXT_CFG_INDEX / 4] = AROS_LONG2LE(((uint32_t)bus << 20) | ((uint32_t)dev << 15) | ((uint32_t)sub << 12));
        volatile uint32_t *data = (volatile uint32_t *)(psd->regs + PCIE2712_EXT_CFG_DATA + (reg & 0xFFC));
        return AROS_LE2LONG(*data);
    }

    return 0xFFFFFFFF;
}

static void WriteConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val)
{
    if (!psd->link_up && bus > 0)
        return;

    if (psd->ecam)
    {
        uint32_t off = ecam_offset(bus, dev, sub, reg & ~3);
        if (off < BCM2712_PCIE0_ECAM_SIZE)
        {
            volatile uint32_t *p = (volatile uint32_t *)(psd->ecam + off);
            *p = AROS_LONG2LE(val);
            return;
        }
    }

    if (psd->regs)
    {
        volatile uint32_t *rc = (volatile uint32_t *)psd->regs;
        rc[PCIE2712_EXT_CFG_INDEX / 4] = AROS_LONG2LE(((uint32_t)bus << 20) | ((uint32_t)dev << 15) | ((uint32_t)sub << 12));
        volatile uint32_t *data = (volatile uint32_t *)(psd->regs + PCIE2712_EXT_CFG_DATA + (reg & 0xFFC));
        *data = AROS_LONG2LE(val);
    }
}

UBYTE PCIBcm2712__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3);
    return (UBYTE)(val >> ((msg->reg & 3) * 8));
}

UWORD PCIBcm2712__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3);
    return (UWORD)(val >> ((msg->reg & 2) * 8));
}

ULONG PCIBcm2712__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

VOID PCIBcm2712__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    ULONG shift = (msg->reg & 3) * 8;
    ULONG mask  = ~(0xFFUL << shift);
    ULONG val   = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    val = (val & mask) | ((ULONG)msg->val << shift);
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3, val);
}

VOID PCIBcm2712__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    ULONG shift = (msg->reg & 2) * 8;
    ULONG mask  = ~(0xFFFFUL << shift);
    ULONG val   = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    val = (val & mask) | ((ULONG)msg->val << shift);
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3, val);
}

VOID PCIBcm2712__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

/*
 * Map PCI Memory (BARs) into CPU virtual address space.
 */
APTR PCIBcm2712__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_MapPCI *msg)
{
    IPTR pci_addr = (IPTR)msg->PCIAddress;
    IPTR cpu_addr;

    /* Outbound memory window translation */
    if (pci_addr >= BCM2712_PCIE_PCI_WIN &&
        pci_addr < (BCM2712_PCIE_PCI_WIN + BCM2712_PCIE_WIN_SIZE))
    {
        cpu_addr = (pci_addr - BCM2712_PCIE_PCI_WIN) + BCM2712_PCIE_CPU_WIN;
    }
    else
    {
        cpu_addr = pci_addr;
    }

    /* Identity map as Device memory; the pointer is the CPU address. */
    if (!KrnMapGlobal((void *)cpu_addr, (void *)cpu_addr, msg->Length,
                      MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
        return NULL;

    return (APTR)cpu_addr;
}

IPTR PCIBcm2712__Hidd_PCIDriver__CPUtoPCI(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_CPUtoPCI *msg)
{
    return (IPTR)msg->address + PSD(cl)->dma_offset;
}

APTR PCIBcm2712__Hidd_PCIDriver__PCItoCPU(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_PCItoCPU *msg)
{
    return (APTR)((IPTR)msg->address - PSD(cl)->dma_offset);
}

IPTR PCIBcm2712__Hidd_PCIDriver__AllocPCIMem(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_AllocPCIMem *msg)
{
    return 0;
}

VOID PCIBcm2712__Hidd_PCIDriver__FreePCIMem(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_FreePCIMem *msg)
{
}

/*
 * Device Subclass MSI/Interrupt Handling
 */
ULONG PCIBcm2712Dev__Hidd_PCIDevice__ObtainVectors(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_ObtainVectors *msg)
{
    return 0;
}

VOID PCIBcm2712Dev__Hidd_PCIDevice__ReleaseVectors(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_ReleaseVectors *msg)
{
}

ULONG PCIBcm2712Dev__Hidd_PCIDevice__GetVectorAttribs(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_GetVectorAttribs *msg)
{
    return 0;
}
